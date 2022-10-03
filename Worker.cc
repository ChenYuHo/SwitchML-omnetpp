#include "Worker.h"
#include "SwitchML_m.h"
#include "ModelStats.h"
#define FMT_HEADER_ONLY
#include "fmt/format.h"
#include "JobDispatcher.h"
using namespace omnetpp;
Define_Module(Worker);

void Worker::initialize() {
//    testSignal = registerSignal("testSignal");
//    emit(testSignal, true);

    MTU = par("MTU");
    num_updates = par("num_updates");
    if (MTU && !num_updates) {
        num_updates = (MTU - (8 + 14 + 20 + 8 + 16 + 4 + 12)) / 4;
    } else if (!MTU && num_updates) {
        MTU = int64_t(8 + 14 + 20 + 8 + 16 + num_updates * 4 + 4 + 12);
    } else if (!MTU && !num_updates) {
        MTU = 1500;
        num_updates = 256;
    } else {
        if (8 + 14 + 20 + 8 + 16 + num_updates * 4 + 4 + 12 > uint64_t(MTU)) {
            EV_WARN
                           << fmt::format(
                                   "num_updates {} cannot fit into MTU {}\n",
                                   num_updates, MTU);
        }
    }
    par("MTU").setIntValue(MTU);
    par("num_updates").setIntValue(num_updates);
    num_slots = par("num_slots");
    EV_INFO << "num_updates " << num_updates << " MTU " << MTU << endl;
    srvProcType = cModuleType::get("TrainingProcess");
    out_gate = gate("port$o");
    ToR = out_gate->getPathEndGate()->getOwnerModule();

    collective_scheduler = getSimulation()->findModuleByPath(
            "<root>.collective_scheduler");
    endTransmissionEvent = new cMessage("endTxEvent", 1);
    channel = out_gate->findTransmissionChannel();
    isBusy = false;
    job_dispatcher = getParentModule()->getSubmodule("job_dispatcher");
}

void Worker::sendNextPacket(SwitchMLPacket *pkt, uint32_t next_offset) {
    auto p = pkt->dup();
    p->setFrom_id(getId());
    p->setVer(1 - pkt->getVer());
    p->setOffset(next_offset);
    p->setUpward(true);
    try_send(p);
    // caller should delete pkt
}

void Worker::try_send(cPacket *pkt) {
    if (endTransmissionEvent->isScheduled()) {
        // We are currently busy, so just queue up the packet.
        pkt->setTimestamp();
        queue.insert(pkt);
    } else {
        // We are idle, so we can start transmitting right away.
        startTransmitting(pkt);
    }
}

void Worker::startTransmitting(cMessage *pkt) {
    isBusy = true;
    send(pkt, out_gate);
    simtime_t endTransmission =
            channel ? channel->getTransmissionFinishTime() : simTime();
    scheduleAt(endTransmission, endTransmissionEvent);
}

void Worker::startOneCollectiveOperation(uint64_t job_id) {

    auto m =
            (CollectiveOperationRequest*) (collective_operation_requests_for_job[job_id].pop());
    doing_collective_operation[job_id] = true;
    auto grad_size = m->getSize();
    auto num_pkts_expected = grad_size / num_updates;
    if (grad_size % num_updates)
        num_pkts_expected += 1;
    EV_DEBUG
                    << fmt::format(
                            "Worker {} startOneCollectiveOperation for job {} grad_size {}, expect {} pkts, queue still has {} reqs",
                            getId(), job_id, grad_size, num_pkts_expected,
                            collective_operation_requests_for_job[job_id].getLength())
                    << endl;
    m->setStart(simTime());
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
        p->setJob_id(m->getJob_id());
        p->setNum_pkts_expected(num_pkts_expected);
        p->setGrad_size(grad_size);
        p->setUpward(true);
        try_send(p);
    }
    active_collective_operation_request_for_job[job_id] = m;
}

void Worker::handleMessage(cMessage *msg) {
    if (!msg->isPacket()) {
        switch (msg->getKind()) {
        case 0: {
            // CollectiveOperationRequest from CollectiveOperationScheduler or TrainingProcess
            auto req = (CollectiveOperationRequest*) msg;
            collective_operation_requests_for_job[req->getJob_id()].insert(req);
            EV_DEBUG
                            << fmt::format(
                                    "Worker {} collective_operation_requests_for_job for job {} queue has {} reqs",
                                    getId(), req->getJob_id(),
                                    collective_operation_requests_for_job[req->getJob_id()].getLength())
                            << endl;
            if (!doing_collective_operation[req->getJob_id()]) {
                startOneCollectiveOperation(req->getJob_id());
            }
            break;
        }
        case 1: {
            // Transmission finished, we can start next one.
            isBusy = false;
            if (!queue.isEmpty()) {
                startTransmitting((cMessage*) queue.pop());
            }
            break;
        }
        case 3: { // new Job
            // mod will self destroy
            auto job = (Job*) (msg);
            cModule *mod = srvProcType->createScheduleInit(
                    fmt::format("Job{}_Rank{}_Worker{}_{}", job->getJob_id(),
                            job->getRank(), getId(), ++num_jobs_given).c_str(),
                    this);
            sendDirect(msg, mod, "directin");
            training_process_for_job[job->getJob_id()] = mod;
            EV_DEBUG << "Worker " << getId() << " Start Server Process for Job "
                            << job->getJob_id() << endl;
            break;
        }
        case 5: { // finished job from TrainingProcess
            auto job = (Job*) msg;
            auto jid = job->getJob_id();
            job->setWorker_id(getId());
            sendDirect(job, job_dispatcher, "directin");
            training_process_for_job[jid]->deleteModule();
            training_process_for_job.erase(jid);
            collective_operation_requests_for_job.erase(jid);
            doing_collective_operation.erase(jid);
            break;
        }
        default:
            delete msg;
            EV_FATAL << "got unexpected message" << endl;
            break;
        }
        return;
    }

    auto p = (SwitchMLPacket*) (msg);
    EV_DEBUG << "Worker " << getId() << " get packet slot " << p->getSlot()
                    << " at " << simTime() << endl;
    auto &set = received_pkts[p->getTensor_key()];
    if (set.find(p->getOffset()) != set.end()) {
        // duplicate
        EV_DEBUG << "Worker " << getId() << " got duplicate packet.\n";
    } else {
        set.insert(p->getOffset());
        EV_DEBUG
                        << fmt::format(
                                "Worker {} done slot {} offset {} pkt {}/{}\n",
                                getId(), p->getSlot(), p->getOffset(),
                                set.size(), p->getNum_pkts_expected());
        // cancel timer if retransmission is enabled
        auto jid = p->getJob_id();
        auto req =
                (CollectiveOperationRequest*) active_collective_operation_request_for_job[jid];
        if (set.size() == p->getNum_pkts_expected()) {
            // collection operation done, notify others
            EV_DEBUG
                            << fmt::format(
                                    "Worker {} done a Collective Operation\n",
                                    getId());
            req->setKind(2);
            auto completed = req->getChunk_id() + 1 == req->getNum_chunks();
            req->setCompleted(completed);
            if (collective_scheduler) {
                sendDirect(req->dup(), collective_scheduler, "directin");
            }

            doing_collective_operation[jid] = false;
            if (completed) {
                sendDirect(req, training_process_for_job[jid], "directin");
                EV_DEBUG
                                << fmt::format(
                                        "Worker {} Job {} done aggregation layer {}\n",
                                        getId(), jid, req->getLayer());
            } else {
                req->setKind(8);
                sendDirect(req, training_process_for_job[jid], "directin");
            }
            if (!collective_operation_requests_for_job[jid].isEmpty()) {
                startOneCollectiveOperation(jid);
            }

            // can't clear yet if loss recovery is enabled
            set.clear();
            auto tensor_key = p->getTensor_key();
            received_pkts.erase(tensor_key);
            ((JobDispatcher*) (job_dispatcher))->clean_resources_for_tensor_key(
                    jid, tensor_key);
        } else {
            auto next_offset = p->getOffset() + num_slots * num_updates;
            if (next_offset < p->getGrad_size()) {
                EV_DEBUG
                                << fmt::format(
                                        "Worker {} Job {} layer {} send next offset {}\n",
                                        getId(), jid, req->getLayer(),
                                        next_offset);
                sendNextPacket(p, next_offset);
            }
        }
    }
    delete p;
}

Worker::~Worker() {
//    emit(testSignal, false);
    cancelAndDelete(endTransmissionEvent);
}
