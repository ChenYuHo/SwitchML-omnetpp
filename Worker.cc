#include "Worker.h"
#include "SwitchML_m.h"
#include "ModelStats.h"
#define FMT_HEADER_ONLY
#include "fmt/format.h"
#include "JobDispatcher.h"
using namespace omnetpp;
Define_Module(Worker);

void Worker::initialize() {
    gbps = int64_t(
            gate("port$o")->getChannel()->par("datarate").doubleValue() / 1e9);
    EV_DEBUG << "gbps " << gbps << endl;
    packet_simulation = par("packet_simulation");
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
    int64_t retransmission_timeout_ms = par("retransmission_timeout");
    retransmission_enabled = retransmission_timeout_ms > 0;
    if (retransmission_enabled) {
        retransmission_timeout = SimTime(retransmission_timeout_ms, SIMTIME_MS);
        EV_INFO << "retransmission time " << retransmission_timeout << endl;
    }
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
    pktOut = registerSignal("pktOut");
    pktRetransmission = registerSignal("pktRetransmission");
    workerQueueLength = registerSignal("workerQueueLength");
//    pktIn = registerSignal("pktIn");

//    auto p = new SwitchMLPacket();
//    p->setPriority(1);
//    queue.insert(p->dup());
//    p->setPriority(3);
//    queue.insert(p->dup());
//    p->setPriority(7);
//    queue.insert(p->dup());
//    p->setPriority(4);
//    queue.insert(p->dup());
//    p->setPriority(-1);
//    queue.insert(p->dup());
//    p->setPriority(5);
//    queue.insert(p->dup());
//    p->setPriority(2);
//    queue.insert(p);
//    while (!queue.empty()) {
//        auto p1 = (SwitchMLPacket*) queue.pop();
//        EV_DEBUG << " " << p1->getPriority();
//        delete p1;
//    }
//    EV_DEBUG << endl;
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

void Worker::try_send(SwitchMLPacket *pkt) {
    if (endTransmissionEvent->isScheduled()) {
        // We are currently busy, so just queue up the packet.
        pkt->setPriority(tensor_priority[pkt->getTensor_key()]);
        queue.insert(pkt);
        emit(workerQueueLength, queue.getLength());
    } else {
        // We are idle, so we can start transmitting right away.
        startTransmitting(pkt);
    }
}

void Worker::startTransmitting(SwitchMLPacket *pkt) {
    while (pkt->getKind() == 10) {
        // retransmission canceled
        delete pkt;
        if (queue.isEmpty()) {
            isBusy = false;
            return;
        }
        pkt = (SwitchMLPacket*) queue.pop();
    }
    isBusy = true;
    if (retransmission_enabled) {
        schedule_timeout_retransmission(pkt);
        if (pkt->getKind() == 9) {
            // This is a retransmission pkt
            pkt->setKind(99); // anything not 9: mark as non retransmission pkts
            emit(pktRetransmission, pkt);
        }
    }
    send(pkt, out_gate);
    emit(pktOut, pkt);
    simtime_t endTransmission =
            channel ? channel->getTransmissionFinishTime() : simTime();
    scheduleAt(endTransmission, endTransmissionEvent);
}

void Worker::schedule_timeout_retransmission(SwitchMLPacket *pkt) {
    auto retransmission_pkt = pkt->dup();
    retransmission_pkt->setKind(9);
    auto &pkts = retransmission_pkts[pkt->getTensor_key()];
    pkts.reserve(num_slots);
    pkts[pkt->getSlot()] = retransmission_pkt;
    scheduleAfter(retransmission_timeout, retransmission_pkt);
}

void Worker::notifyCollectiveOperationDone(CollectiveOperationRequest *req) {
    EV_DEBUG << fmt::format("Worker {} done a Collective Operation", getId())
                    << " at " << simTime() << endl;
    req->setKind(2);
    auto &tensor_key = req->getTensor_key();
    auto jid = tensor_key.job_id;
    auto completed = req->getChunk_id() + 1 == req->getNum_chunks();
    req->setCompleted(completed);
    if (collective_scheduler) {
        sendDirect(req->dup(), collective_scheduler, "directin");
    }

    doing_collective_operation[jid] = false;
    if (completed) {
        iter_of_tkey[tensor_key] += 1;
        chunk_of_tkey[tensor_key] = 0;
        sendDirect(req, training_process_for_job[jid], "directin");
        EV_DEBUG
                        << fmt::format(
                                "Worker {} Job {} done aggregation layer {}\n",
                                getId(), jid, tensor_key.layer);
    } else {
        chunk_of_tkey[tensor_key] += 1;
        req->setKind(8);
        EV_DEBUG
                        << fmt::format(
                                "Worker {} Job {} done collective operation chunk layer {}\n",
                                getId(), jid, tensor_key.layer);
        sendDirect(req, training_process_for_job[jid], "directin");
    }
    if (!collective_operation_requests_for_job[jid].isEmpty()) {
        startOneCollectiveOperation(jid);
    } else {
        EV_DEBUG
                        << fmt::format(
                                "Worker {} Job {} not starting collective operations\n",
                                getId(), jid);
    }
    ((JobDispatcher*) (job_dispatcher))->clean_resources_for_tensor_key(jid,
            tensor_key);
}

void Worker::startOneCollectiveOperation(uint64_t job_id) {

    auto m =
            (CollectiveOperationRequest*) (collective_operation_requests_for_job[job_id].pop());
    doing_collective_operation[job_id] = true;
    auto grad_size = m->getSize();
    m->setStart(simTime());
    if (packet_simulation) {
        auto num_pkts_expected = grad_size / num_updates;
        if (grad_size % num_updates)
            num_pkts_expected += 1;
        EV_DEBUG
                        << fmt::format(
                                "Worker {} startOneCollectiveOperation for job {} grad_size {}, expect {} pkts, queue still has {} reqs",
                                getId(), job_id, grad_size, num_pkts_expected,
                                collective_operation_requests_for_job[job_id].getLength())
                        << " at " << simTime() << endl;
        // first batch
        auto &tensor_key = m->getTensor_key();
        for (uint64_t slot = 0; slot < num_slots; ++slot) {
            auto offset = slot * num_updates;
            if (offset >= grad_size)
                break;
            auto p = new SwitchMLPacket();
            p->setPriority(m->getPriority());
            p->setByteLength(MTU);
            p->setFrom_id(getId());
            p->setSlot(slot);
            p->setVer(0);
            p->setOffset(offset);
            p->setTensor_key(tensor_key);
            p->setN_workers(m->getNum_workers_allocated());
            p->setNum_pkts_expected(num_pkts_expected);
            p->setGrad_size(grad_size);
            p->setUpward(true);
            p->setIter(iter_of_tkey[tensor_key]);
            p->setChunk(chunk_of_tkey[tensor_key]);
            try_send(p);
        }
        active_collective_operation_request_for_job[job_id] = m;
    } else {
        EV_DEBUG
                        << fmt::format(
                                "Worker {} startOneCollectiveOperation for job {} grad_size {}, queue still has {} reqs",
                                getId(), job_id, grad_size,
                                collective_operation_requests_for_job[job_id].getLength())
                        << endl;
        m->setKind(7);
        EV_DEBUG << "time  "
                        << SimTime(grad_size * 4 * 8 * 1000 / gbps, SIMTIME_PS)
                        << endl;
        scheduleAfter(SimTime(grad_size * 4 * 8 * 1000 / gbps, SIMTIME_PS), m);
    }
}

void Worker::handleMessage(cMessage *msg) {
    if (!msg->isPacket()) {
        switch (msg->getKind()) {
        case 0: {
            // CollectiveOperationRequest from CollectiveOperationScheduler or TrainingProcess
            auto req = (CollectiveOperationRequest*) msg;
            tensor_priority[req->getTensor_key()] = req->getPriority();
            auto jid = req->getTensor_key().job_id;
            collective_operation_requests_for_job[jid].insert(req);
            EV_DEBUG
                            << fmt::format(
                                    "Worker {} collective_operation_requests_for_job for job {} queue has {} reqs",
                                    getId(), jid,
                                    collective_operation_requests_for_job[jid].getLength())
                            << endl;
            if (!doing_collective_operation[jid]) {
                startOneCollectiveOperation(jid);
            }
            break;
        }
        case 1: {
            // Transmission finished, we can start next one.
            isBusy = false;
            if (!queue.isEmpty()) {
                startTransmitting((SwitchMLPacket*) queue.pop());
            }
            break;
        }
        case 3: { // new Job
            // mod will self destroy
            auto job = (Job*) (msg);
            cModule *mod = srvProcType->createScheduleInit(
                    fmt::format("Job{}_Rank{}_Worker{}_TrainingProcess{}",
                            job->getJob_id(), job->getRank(), getId(),
                            ++num_jobs_given).c_str(), this);
            sendDirect(msg, mod, "directin");
            training_process_for_job[job->getJob_id()] = mod;
            break;
        }
        case 5: { // finished job from TrainingProcess
            auto job = (Job*) msg;
            auto jid = job->getJob_id();
            job->setWorker_id(getId());
            sendDirect(job, job_dispatcher, "directin");
            training_process_for_job[jid]->callFinish();
            training_process_for_job[jid]->deleteModule();
            training_process_for_job.erase(jid);
            collective_operation_requests_for_job.erase(jid);
            doing_collective_operation.erase(jid);
            break;
        }
        case 7: { // Collective operation done for non packet simulations
            notifyCollectiveOperationDone((CollectiveOperationRequest*) msg);
            break;
        }
        case 14: { // Update tensor priority
            auto req = (CollectiveOperationRequest*) msg;
            tensor_priority[req->getTensor_key()] = req->getPriority();
            delete req;
            break;
        }
        case 17: {
            // compress
            auto req = (CollectiveOperationRequest*) msg;
            auto sz = req->getSize();
            req->setSize(sz > 3 ? sz / 4 : 1); // CNat compress size
            req->setKind(0);
            scheduleAfter(SimTime(double(sz) / 44.525520170, SIMTIME_NS), req); // CNat compress time
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

    if (retransmission_enabled) {
        if (p->getKind() == 9) {
            // Timer for timeout retransmission pkt fired! so do retransmission
            EV_DEBUG << "Worker " << getId()
                            << " try_send retransmission packet slot "
                            << p->getSlot() << " at " << simTime() << endl;
            try_send(p);
            return;

        }

        // otherwise, need to cancel previously scheduled retransmission
        auto &tensor_key = p->getTensor_key();
        if (p->getIter() == iter_of_tkey[tensor_key] ?
                p->getChunk() < chunk_of_tkey[tensor_key] :
                p->getIter() < iter_of_tkey[tensor_key]) {
            auto rpkt = retransmission_pkts[tensor_key][p->getSlot()];
            if (queue.contains(rpkt)) {
                rpkt->setKind(10);
                // if not scheduled yet, mark so it will be canceled when queue pops
                // if scheduled (sent), let it be
            }
            delete p;
            return;
        }
    }

//    emit(pktIn, p);
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
        if (retransmission_enabled) {
            auto rpkt = retransmission_pkts[p->getTensor_key()][p->getSlot()];
            if (queue.contains(rpkt)) {
                rpkt->setKind(10);
                // if not scheduled yet, mark so it will be canceled when queue pops

                // if scheduled (sent), let it be
            }
        }

        auto &tensor_key = p->getTensor_key();
        if (set.size() == p->getNum_pkts_expected()) {
            // collective operation done!
            notifyCollectiveOperationDone(
                    (CollectiveOperationRequest*) active_collective_operation_request_for_job[tensor_key.job_id]);

            // clear local resources
            // everything received that has timestamps before this obsolete_pkt_timestamp is obsolete
//            if (retransmission_enabled) {
//                obsolete_pkt_timestamp[tensor_key] = simTime()
//                        + SimTime(1, SIMTIME_PS);
//            }
            set.clear();
            received_pkts.erase(tensor_key);
        } else {
            // send next
            auto next_offset = p->getOffset() + num_slots * num_updates;
            if (next_offset < p->getGrad_size()) {
                EV_TRACE
                                << fmt::format(
                                        "Worker {} Job {} layer {} send next offset {}\n",
                                        getId(), tensor_key.job_id,
                                        tensor_key.layer, next_offset);
                sendNextPacket(p, next_offset);
            }
        }
    }
    delete p;
}

Worker::~Worker() {
//    emit(testSignal, false);
    cancelAndDelete(endTransmissionEvent);
    if (retransmission_enabled) {
        for (auto &pair : retransmission_pkts) {
            for (auto p : pair.second) {
                cancelAndDelete(p);
            }
        }
    }
}
