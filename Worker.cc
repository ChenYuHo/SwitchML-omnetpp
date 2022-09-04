#include "Worker.h"
#include "SwitchML_m.h"
#include "ModelStats.h"
#define FMT_HEADER_ONLY
#include "fmt/format.h"
using namespace omnetpp;
Define_Module(Worker);

void Worker::initialize() {
    MTU = par("MTU");
    num_updates = par("num_updates");
    if (MTU && !num_updates) {
        num_updates = (MTU - (8 + 14 + 20 + 8 + 16 + 4 + 12)) / 4;
    } else if (num_updates) {
        MTU = 8 + 14 + 20 + 8 + 16 + num_updates * 4 + 4 + 12;
    } else {
        MTU = 1500;
        num_updates = 256;
    }
    num_slots = par("num_slots");
    free_gpus = par("num_gpus");
    srvProcType = cModuleType::get("TrainingProcess");
    out_gate = gate("port$o");

    collective_scheduler = getSimulation()->findModuleByPath(
            "<root>.collective_scheduler");
}

void Worker::sendNextPacket(SwitchMLPacket *pkt, uint32_t next_offset) {
    auto p = pkt->dup();
    p->setFrom_id(getId());
    p->setVer(1 - pkt->getVer());
    p->setOffset(next_offset);
    p->setUpward(true);
//    EV_DEBUG << "Worker send next packet\n";
    send(p, out_gate);
    // caller should delete pkt
}

void Worker::startOneCollectiveOperation(uint64_t job_id) {

    auto m = (CollectiveOperationRequest*) (collective_operation_requests_for_job[job_id].pop());
    doing_collective_operation[job_id] = true;
    auto grad_size = m->getSize();
    auto num_pkts_expected = grad_size / num_updates;
    if (grad_size % num_updates)
        num_pkts_expected += 1;
    EV_DEBUG << fmt::format("Worker {} startOneCollectiveOperation for job {} grad_size {}, expect {} pkts, queue still has {} reqs", getId(), job_id, grad_size, num_pkts_expected, collective_operation_requests_for_job[job_id].getLength()) << endl;
    for (uint64_t slot = 0; slot < num_slots; ++slot) {
        auto offset = slot * num_updates;
        if (offset >= grad_size)
            break;
        auto p = new SwitchMLPacket();
        p->setByteLength(MTU);
        p->setFrom_id(getId());
        p->setSlot(slot);
        p->setVer(0);
        p->setOffset(offset);
        p->setTensor_key(m->getTensor_key());
        p->setN_workers(m->getNum_workers_allocated());
        p->setLayer(m->getLayer());
        p->setJob_id(m->getJob_id());
        p->setNum_pkts_expected(num_pkts_expected);
        p->setGrad_size(grad_size);
        p->setNum_chunks(m->getNum_chunks());
        p->setChunk_id(m->getChunk_id());
        p->setModel(m->getModel());
        p->setUpward(true);
//        EV_DEBUG << "Worker send packet\n";
        send(p, out_gate);
    }
    delete m;
}

void Worker::handleMessage(cMessage *msg) {
    if (!msg->isPacket()) {
        switch (msg->getKind()) {
        case 0: {
            // CollectiveOperationRequest from CollectiveOperationScheduler or TrainingProcess
            auto req = (CollectiveOperationRequest*) msg;
            collective_operation_requests_for_job[req->getJob_id()].insert(req);
            EV_DEBUG << fmt::format("Worker {} collective_operation_requests_for_job for job {} queue has {} reqs", getId(), req->getJob_id(), collective_operation_requests_for_job[req->getJob_id()].getLength()) << endl;
            if (!doing_collective_operation[req->getJob_id()]) {
                startOneCollectiveOperation(req->getJob_id());
            }
            break;
        }
        case 2: {
            // LayerAck from self, direct to TrainingProcess
            auto ack = (LayerAck*) msg;
            sendDirect(ack, training_process_for_job[ack->getJob_id()], "directin");
            break;
        }
        case 3: { // new Job
            // mod will self destroy
            cModule *mod = srvProcType->createScheduleInit(
                    fmt::format("Worker{}_Job{}", getId(),
                            ++num_jobs_given).c_str(), this);
            sendDirect(msg, mod, "directin");
            auto job = check_and_cast<Job*>(msg);
            free_gpus += job->getGpu();
            training_process_for_job[job->getJob_id()] = (TrainingProcess*) mod;
            EV_DEBUG << "Worker " << getId() << " Start Server Process for Job "
                            << job->getJob_id() << endl;
            break;
        }
        case 4: { // HierarchyQuery
            auto q = (HierarchyQuery*) msg;
            q->appendPath(getId());
            q->appendModules(this);
            q->setNum_gpus(free_gpus);
            job_dispatcher = getSimulation()->getModule(q->getFrom_id());
            sendDirect(q, out_gate->getPathEndGate()->getOwnerModule(), "directin");
            break;
        }
        case 5: { // finished job from TrainingProcess
            auto job  = (Job*) msg;
            job->setWorker_id(getId());
            sendDirect(job, job_dispatcher, "directin");
            break;
        }
        default:
            delete msg;
            EV_FATAL << "got unexpected message" << endl;
            break;
        }

    } else {
        auto p = (SwitchMLPacket*) (msg);
        auto &set = received_pkts[p->getTensor_key()];
        if (set.find(p->getOffset()) != set.end()) {
            // duplicate
            EV_DEBUG << "Worker " << getId() << " got duplicate packet.\n";
        } else {
            set.insert(p->getOffset());
            // cancel timer if retransmission is enabled
            if (set.size() == p->getNum_pkts_expected()) {
                // allreduce done, notify allreducer
                EV_DEBUG << fmt::format("Worker {} done allreduce\n", getId());
//                auto training_process = training_process_for_job[p->getJob_id()];
                auto completed = p->getChunk_id() + 1 == p->getNum_chunks();
                auto ack = new LayerAck();
                ack->setLayer(p->getLayer());
                ack->setJob_id(p->getJob_id());
//                auto w = wu_time(p->getModel(), p->getLayer());
//                ack->setWeight_update_time(w);
                ack->setCompleted(completed);
                if (collective_scheduler) {
                    auto dup = ack->dup();
                    dup->setKind(1);
                    sendDirect(dup, collective_scheduler, "directin");
                }

                doing_collective_operation[p->getJob_id()] = false;
                if (completed) {
                    // after weight update, notify TrainingProcess this allreduce completes
                    ack->setKind(2);
                    scheduleAfter(SimTime(wu_time(p->getModel(), p->getLayer()), SIMTIME_PS), ack);
                }
                if (!collective_operation_requests_for_job[p->getJob_id()].isEmpty()) {
                    startOneCollectiveOperation(p->getJob_id());
                }

                // can't clear yet if loss recovery is enabled
                set.clear();
                received_pkts.erase(p->getTensor_key());
            } else {
                EV_DEBUG << "Worker " << getId() << " " << __LINE__ << endl;
                auto next_offset = p->getOffset() + num_slots * num_updates;
                if (next_offset < p->getGrad_size()) {
                    sendNextPacket(p, next_offset);
                }
            }
        }
        delete p;
    }
}

