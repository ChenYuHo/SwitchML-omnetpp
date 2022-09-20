#include "TrainingProcess2.h"
#include "ModelStats.h"
//#include "Worker.h"
#include <sstream>
#include <vector>

Define_Module(TrainingProcess2);

void TrainingProcess2::process_ack(LayerAck *ack) {
    can_do_fp[ack->getLayer()] = true;
    if (++count == num_layers) { // this iter finishes
        count = 0;
        emit(iterTimeSignal, simTime() - iter_start.front());
        iter_start.pop();
    }
    delete ack;
}

void TrainingProcess2::waitAndProcessAck(simtime_t wait_time,
        cQueue *AckQueue) {
    waitAndEnqueue(wait_time, AckQueue);
    while (!AckQueue->isEmpty()) {
        process_ack((LayerAck*) (AckQueue->pop()));
    }
}

void TrainingProcess2::initialize() {

}

void TrainingProcess2::allreduce(uint64_t layer, uint64_t iter) {
    auto size = model[layer];
    auto req = new CollectiveOperationRequest();
    req->setKind(0);
    req->setTraining_process_id(getId());
    req->setWorker_id(worker->getId());
    req->setModel(job->getModel());
    req->setSize(size);
    req->setRank(job->getRank());
    req->setLayer(layer);
    req->setTensor_key(
            hasher(
                    fmt::format("jid{}tid{}iter{}", job->getJob_id(), layer,
                            iter)));
    req->setJob_id(job->getJob_id());
    req->setNum_workers_allocated(job->getNum_workers_allocated());
    if (collective_scheduler) {
        EV_DEBUG << "Enqueue Allreduce" << endl;
        sendDirect(req, collective_scheduler, "directin");
    } else { // send directly to Worker
        EV_DEBUG
                        << fmt::format(
                                "TrainingProcess start allreduce for job {} layer {} size {} iter {}\n",
                                job->getJob_id(), layer, size, iter);
        sendDirect(req, getParentModule(), "directin");
    }
}

std::vector<int64_t> split_int(const char *s, char delim = ',') {
    std::stringstream ss(s);
    std::string item;
    std::vector<int64_t> result;
    while (getline(ss, item, delim)) {
        result.push_back(std::stoll(item));
    }
    return result;
}

std::vector<simtime_t> split_simtime(const char *s, char delim = ',') {
    std::stringstream ss(s);
    std::string item;
    std::vector<simtime_t> result;
    while (getline(ss, item, delim)) {
        result.push_back(SimTime(std::stoull(item), SIMTIME_PS));
    }
    return result;
}

void TrainingProcess2::handleMessage(cMessage *msg) {
    switch (msg->getKind()) {
    case 3: {
        // initial job msg from worker
        job = (Job*) msg;
        collective_scheduler = getSimulation()->findModuleByPath(
                "<root>.collective_scheduler");
        job_dispatcher = getSimulation()->findModuleByPath(
                "<root>.job_dispatcher");
        worker = getParentModule();
        if (collective_scheduler) {
            EV_DEBUG << "Collective Scheduler is "
                            << collective_scheduler->getFullName() << endl;
        } else
            EV_DEBUG << "No Collective Scheduler" << endl;

        rank = job->getRank();
        jid = job->getJob_id();
        wid = worker->getId();
        iters = job->getIters();
        distributed = job->getNum_workers_allocated() > 1;

        if (job_dispatcher->par("custom_model").boolValue()) {
            model = split_int(
                    job_dispatcher->par("custom_model_sizes").stringValue());
            fp_times = split_simtime(
                    job_dispatcher->par("custom_fp_times").stringValue());
            bp_times = split_simtime(
                    job_dispatcher->par("custom_bp_times").stringValue());
            wu_times = split_simtime(
                    job_dispatcher->par("custom_wu_times").stringValue());
            num_layers = model.size();
            fp_times.resize(num_layers);
            bp_times.resize(num_layers);
            wu_times.resize(num_layers);
        } else {
            model = std::vector<int64_t> { 100, 100 };
            fp_times = std::vector<simtime_t> { SimTime(1, SIMTIME_MS), SimTime(
                    1, SIMTIME_MS) };
            bp_times = std::vector<simtime_t> { SimTime(1, SIMTIME_MS), SimTime(
                    1, SIMTIME_MS) };
            wu_times = std::vector<simtime_t> { SimTime(1, SIMTIME_MS), SimTime(
                    1, SIMTIME_MS) };
            num_layers = model.size();
        }

        can_do_fp.resize(num_layers, true);
        layer_done.resize(num_layers, false);

        auto ins = new Instruction("fp", 20);
        ins->setLayer(0);
        ins->setFp(true);
        ins->setIter(0);
        scheduleAt(simTime(), ins);
        break;
    }
    case 20: {
        auto ins = (Instruction*) msg;
        auto layer = ins->getLayer();
        if (ins->getFp()) {
            // FP layer "layer-1" done, try to start layer "layer"
            if (iter > 0 && layer == 0) {
                EV_DEBUG
                                << fmt::format(
                                        "Worker {} Job {} iter {} done bp layer {} ",
                                        wid, jid, iter, layer) << simTime()
                                << endl;
            }

            if (!can_do_fp[layer]) {
                // waiting for communication, so idling
                idle_start.push(simTime());
                break;
            } else if (!idle_start.empty()) {
                // record idle time
            }
            can_do_fp[layer] = false;
            if (layer == 0) {
                iter_start.push(simTime());
            } else {
                EV_DEBUG
                                << fmt::format(
                                        "Worker {} Job {} iter {} done fp layer {} ",
                                        wid, jid, iter, layer) << simTime()
                                << endl;
            }
            ins->setLayer(layer + 1);
            if (layer + 1 == num_layers) {
                ins->setFp(false);
            }
            scheduleAfter(fp_times[layer], msg);
//            EV_DEBUG << "start fp layer " << layer << " / " << num_layers << " "
//                                        << fp_times[layer] << " at " << simTime() << endl;
        } else {
            if (layer == num_layers) {
                EV_DEBUG
                                << fmt::format(
                                        "Worker {} Job {} iter {} done fp layer {} ",
                                        wid, jid, iter, layer - 1) << simTime()
                                << endl;
            } else {
                EV_DEBUG
                                << fmt::format(
                                        "Worker {} Job {} iter {} done bp layer {} ",
                                        wid, jid, iter, layer) << simTime()
                                << endl;
            }
            if (layer < num_layers) {
                // start or enqueue collective of layer "layer"
                if (distributed) {
                    if (layer == num_layers - 1) {
                        comm_start.push(simTime());
                    }
//                    EV_DEBUG << "start comm layer " << layer << " at "
//                                    << simTime() << endl;
                    allreduce(layer, ins->getIter());
                } else {
                    auto ack = new LayerAck();
                    ack->setLayer(layer);
                    scheduleAfter(wu_times[layer], ack);
//                    EV_DEBUG << "start wu layer " << layer << " "
//                                    << wu_times[layer] << endl;
                }
            }
            if (layer == 0) {
                ins->setFp(true);
                ins->setIter(ins->getLayer() + 1);
                scheduleAt(simTime(), ins);
            } else {
                ins->setLayer(layer - 1);
                scheduleAfter(bp_times[layer - 1], msg);
//                EV_DEBUG << "start bp layer " << layer - 1 << " "
//                                << bp_times[layer - 1] << " at " << simTime()
//                                << endl;
            }
        }
        break;
    }
    case 2: {
        // LayerAck meaning collective and wu is done
        auto ack = (LayerAck*) msg;
        can_do_fp[ack->getLayer()] = true;
        layer_done[ack->getLayer()] = true;
        if (iter + 1 < iters) {
            auto ins = new Instruction("fp", 20);
            ins->setLayer(0);
            ins->setFp(true);
            ins->setIter(iter + 1);
            scheduleAt(simTime(), ins);
        }
        if (std::all_of(layer_done.cbegin(), layer_done.cend(), [](bool done) {
            return done;
        })) {
            // iter end time
            iter++;
            if (iter == iters) {
                EV_DEBUG << "rank " << rank << " done job " << jid << " at "
                                << simTime() << endl;
                job->setFinish_time(simTime());
                job->setKind(5);
                sendDirect(job, getParentModule(), "directin");
                deleteModule();
            }
        }

//        sendDirect(ack, training_process_for_job[ack->getJob_id()], "directin");
        break;
    }
    case 8: {
        // partial completion, chunks
        delete msg;
        break;
    }
    case 5: { // finished job from TrainingProcess2
//        auto job = (Job*) msg;
//        auto jid = job->getJob_id();
//        job->setWorker_id(getId());
//        sendDirect(job, job_dispatcher, "directin");
//        training_process_for_job.erase(jid);
//        collective_operation_requests_for_job.erase(jid);
//        doing_collective_operation.erase(jid);
        break;
    }
    default:

        EV_FATAL << "got unexpected message " << msg->getKind() << endl;
        delete msg;
        break;
    }
}

void TrainingProcess2::finish() {
}

TrainingProcess2::~TrainingProcess2() {
}
