#include "TrainingProcess.h"
#include "ModelStats.h"
#include <sstream>
#include <vector>

Define_Module(TrainingProcess);

void TrainingProcess::initialize() {
    fullIterTime = registerSignal("fullIterTime");
    contiguousIterTime = registerSignal("contiguousIterTime");
    idleTime = registerSignal("idleTime");
    idleTimeWu = registerSignal("idleTimeWu");
    commTime = registerSignal("commTime");
    realCommTime = registerSignal("realCommTime");
    datarate = // Gbps
            uint64_t(
                    getParentModule()->gate("port$o")->getChannel()->par(
                            "datarate").doubleValue()/* bps */) / 1000000000;
}

void TrainingProcess::startComm(uint64_t layer, uint64_t iter) {
    if (distributed) {
        if (layer == num_layers - 1) {
            comm_start.push(simTime());
        }

        //TODO: enqueue is not start
        EV_DEBUG
                        << fmt::format(
                                "Worker {} Rank {} Job {} iter {} start comm layer {} ",
                                wid, rank, jid, iter, layer) << simTime()
                        << endl;
        auto size = model[layer];
        auto req = new CollectiveOperationRequest();
        req->setKind(0);
        req->setTraining_process_id(getId());
        req->setWorker_id(wid);
        req->setModel(job->getModel());
        req->setSize(size);
        req->setRank(rank);
        req->setLayer(layer);
        req->setTensor_key(
                hasher(fmt::format("jid{}tid{}", job->getJob_id(), layer)));
        req->setJob_id(jid);
        req->setNum_workers_allocated(num_workers_allocated);
        if (collective_scheduler) {
            EV_DEBUG
                            << fmt::format(
                                    "Worker {} Rank {} Job {} iter {} enqueue collective operation for layer {} size {} ",
                                    wid, rank, jid, iter, layer, size)
                            << simTime() << endl;
            sendDirect(req, collective_scheduler, "directin");
        } else { // send directly to Worker
            EV_DEBUG
                            << fmt::format(
                                    "Worker {} Rank {} Job {} iter {} TrainingProcess start collective operation for layer {} size {} ",
                                    wid, rank, jid, iter, layer, size)
                            << simTime() << endl;
            sendDirect(req, getParentModule(), "directin");
        }

    } else {
        auto req = new CollectiveOperationRequest("req", 4);
        req->setLayer(layer);
        scheduleAfter(wu_times[layer], req);
        EV_DEBUG
                        << fmt::format(
                                "Worker {} Rank {} Job {} iter {} start wu layer {} ",
                                wid, rank, jid, iter, layer) << simTime()
                        << endl;
        idleEnds_wu();
        busy_wu += 1;
        record_idle_wu();
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

void TrainingProcess::idleStarts() {
//    EV_DEBUG << "start idle\n";
//    // just finished any but last fp or last bp
//    if (idle_start.empty()) {
//        idle_start.push(simTime());
//    } else {
//        EV_FATAL << "shouldn't already be idling\n";
////        exit(1);
//    }
}

void TrainingProcess::idleEnds() {
//    // going to start an fp
//    EV_DEBUG << "end idle\n";
//    if (!idle_start.empty()) {
//        // signal?
//        total_idle_time += simTime() - idle_start.front();
//        idle_start.pop();
//    } // else it's already busy
}

void TrainingProcess::idleStarts_wu() {
//    EV_DEBUG << "start idle wu\n";
//    // just finished any but last fp, any wu, or last bp
//    if (idle_start_wu.empty()) {
//        idle_start_wu.push(simTime());
//    }
}

void TrainingProcess::idleEnds_wu() {
//    EV_DEBUG << "end idle wu\n";
//    // going to start an fp or wu
//    if (!idle_start_wu.empty()) {
//        // signal?
//        total_idle_time_wu += simTime() - idle_start_wu.front();
//        idle_start_wu.pop();
//    }
}

//void TrainingProcess::startComm() {
//
//}

void TrainingProcess::start_idle() {
//    if (!busy) {
//        EV_DEBUG << "start idle\n";
//        idle_start.push(simTime());
//    }
}

void TrainingProcess::start_idle_wu() {
//    if (busy_wu == 0) {
//        EV_DEBUG << "start idle wu\n";
//        idle_start_wu.push(simTime());
//    }
}

void TrainingProcess::record_idle() {
//    EV_DEBUG << "end idle\n";
//    total_idle_time += simTime() - idle_start.front();
//    idle_start.pop();
}

void TrainingProcess::record_idle_wu() {
//    EV_DEBUG << "end idle wu\n";
//    total_idle_time_wu += simTime() - idle_start_wu.front();
//    idle_start_wu.pop();
}

void TrainingProcess::handleMessage(cMessage *msg) {
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
        num_workers_allocated = job->getNum_workers_allocated();
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
            min_wait_times.resize(num_layers);
            for (const auto &fp : fp_times) {
                all_fps_and_last_bp_time += fp;
                total_fp_bp_times += fp;
            }
            for (const auto &bp : bp_times) {
                total_fp_bp_times += bp;
            }
            all_fps_and_last_bp_time += bp_times[bp_times.size() - 1];
            uint64_t custom_iters = job_dispatcher->par("custom_iters");
            if (custom_iters)
                iters = custom_iters;
        } else {
            auto m = job->getModel();
            num_layers = n_layers(m);
            model.reserve(num_layers);
            fp_times.reserve(num_layers);
            bp_times.reserve(num_layers);
            wu_times.reserve(num_layers);
            min_wait_times.reserve(num_layers);
            for (size_t layer = 0; layer < num_layers; ++layer) {
                model.push_back(model_size(m, layer));
                fp_times.push_back(SimTime(fp_time(m, layer), SIMTIME_PS));
                bp_times.push_back(SimTime(bp_time(m, layer), SIMTIME_PS));
                wu_times.push_back(SimTime(wu_time(m, layer), SIMTIME_PS));
                min_wait_times.push_back(
                        SimTime(min_wait_time(m, layer, datarate), SIMTIME_PS));
                total_fp_bp_times += SimTime(
                        fp_time(m, layer) + bp_time(m, layer), SIMTIME_PS);
            }
            all_fps_and_last_bp_time = SimTime(all_fps_and_last_bp(m),
                    SIMTIME_PS);
        }

        can_do_fp.resize(num_layers, 1); // for first iter, only require one fp ack ("wu" already done)
        layer_done.resize(num_layers, false);
        real_comm_times.resize(num_layers, 0);

        EV_DEBUG
                        << fmt::format(
                                "Start {}Job {} as rank {} iters {} num_layers {}",
                                distributed ? "distributed " : "", jid, rank,
                                iters, num_layers) << endl;
        auto ins = new Instruction("fp", 20);
        scheduleAt(simTime(), ins);
        break;
    }
    case 20: {
        auto ins = (Instruction*) msg;
        auto layer = ins->getLayer();
        auto iter = ins->getIter();
        // FP layer "layer-1" (or BP layer 0 if layer==0) done, try to start layer "layer"
        if (layer == 0 && iter > 0) {
            // last bp layer
            EV_DEBUG
                            << fmt::format(
                                    "Worker {} Rank {} Job {} iter {} done bp layer {} ",
                                    wid, rank, jid, iter, layer) << simTime()
                            << endl;

            if (iter == iters) {
                // last bp is done, only last comm and wu left for this job
                delete ins;
                break;
            }
            idleStarts();
            idleStarts_wu();
        }
        can_do_fp[layer] += 1;

        if (can_do_fp[layer] < 2) {
            EV_DEBUG << "can't do fp " << layer << " iter " << iter << endl;
            delete ins;
            break;
        } else {
            EV_DEBUG << "can do fp " << layer << " iter " << iter << endl;
        }
        can_do_fp[layer] = 0;

        auto next_layer = layer + 1;
        if (layer == 0) {
            EV_DEBUG
                            << fmt::format(
                                    "Worker {} Rank {} Job {} start iter {} ",
                                    wid, rank, jid, iter) << simTime() << endl;

            iter_start.push(simTime());
        } else {
            EV_DEBUG
                            << fmt::format(
                                    "Worker {} Rank {} Job {} iter {} done fp layer {} ",
                                    wid, rank, jid, iter, layer - 1)
                            << simTime() << endl;
            busy = false;
            busy_wu -= 1;
            start_idle();
            start_idle_wu();

            if (next_layer != num_layers && can_do_fp[next_layer] < 2) {
                idleStarts();
                idleStarts_wu();
            }
        }

        if (next_layer == num_layers) {
            // last fp, proceed to bp
            ins->setKind(21);
        } else {
            ins->setLayer(next_layer);
        }
        scheduleAfter(fp_times[layer], ins);
        busy_wu += 1;
        busy = true;
        EV_DEBUG
                        << fmt::format(
                                "Worker {} Rank {} Job {} iter {} start fp layer {} ",
                                wid, rank, jid, iter, layer) << simTime()
                        << endl;
        job_start_time = simTime();
        break;
    }
    case 21: {
        auto ins = (Instruction*) msg;
        auto layer = ins->getLayer();
        bool first_bp = layer == num_layers - 1;
        if (first_bp) {
            // just done last fp layer
            EV_DEBUG
                            << fmt::format(
                                    "Worker {} Rank {} Job {} iter {} done fp layer {} ",
                                    wid, rank, jid, iter, layer) << simTime()
                            << endl;
//            if (layer < num_layers && can_do_fp[layer + 1] < 2) {
//                idleStarts();
//                idleStarts_wu();
//            }

        } else {
            // just done a bp layer
            EV_DEBUG
                            << fmt::format(
                                    "Worker {} Rank {} Job {} iter {} done bp layer {} ",
                                    wid, rank, jid, iter, layer + 1)
                            << simTime() << endl;
            // start or enqueue collective of layer "layer+1"
            auto comm_layer = layer + 1;
            startComm(comm_layer, ins->getIter());
        }
        if (layer == 0) {
            // last bp
            auto next_iter = ins->getIter() + 1;
            ins->setKind(20); // maybe can start fp
            ins->setIter(next_iter);
            auto ack = ins->dup();
            ins->setKind(22);
            scheduleAfter(bp_times[layer], ack);
        } else {
            ins->setLayer(layer - 1);
        }
        scheduleAfter(bp_times[layer], ins);
        EV_DEBUG
                        << fmt::format(
                                "Worker {} Rank {} Job {} iter {} start bp layer {} ",
                                wid, rank, jid, iter, layer) << simTime()
                        << endl;
        break;
    }
    case 22: {
        // last bp (layer 0)
        auto ins = (Instruction*) msg;
        auto iter = ins->getIter();
        EV_DEBUG
                        << fmt::format(
                                "Worker {} Rank {} Job {} iter {} done bp layer {} ",
                                wid, rank, jid, iter, 0) << simTime() << endl;
        busy = false;
        busy_wu -= 1;
        startComm(0, iter);
        delete ins;
        break;
    }
    case 2: {
        // CollectiveOperationRequest meaning collective is done, schdule after wu
        msg->setKind(4);
        auto req = (CollectiveOperationRequest*) msg;
        scheduleAfter(wu_times[req->getLayer()], req);
        auto layer = req->getLayer();
        real_comm_times[layer] += (simTime() - req->getStart());
        EV_DEBUG
                        << fmt::format(
                                "Worker {} Rank {} Job {} iter {} done comm layer {} ",
                                wid, rank, jid, iter, layer) << simTime()
                        << endl;
        EV_DEBUG
                        << fmt::format(
                                "Worker {} Rank {} Job {} iter {} start wu layer {} ",
                                wid, rank, jid, iter, layer) << simTime()
                        << endl;
        busy_wu += 1;
        break;
    }
    case 4: {
        auto req = (CollectiveOperationRequest*) msg;
        auto layer = req->getLayer();
        EV_DEBUG
                        << fmt::format(
                                "Worker {} Rank {} Job {} iter {} done wu layer {} ",
                                wid, rank, jid, iter, layer) << simTime()
                        << endl;
        if (layer == 0) {
            auto contiguous_iter_time = simTime() - iter_start.front();
            emit(contiguousIterTime, contiguous_iter_time);
            emit(idleTime, contiguous_iter_time - total_fp_bp_times);
        }
        busy_wu -= 1;
        start_idle_wu();
        idleStarts_wu();
        layer_done[layer] = true;
        if (iter + 1 < iters) {
            // start next iteration
            auto ins = new Instruction("fp", 20);
            ins->setLayer(layer);
            ins->setIter(iter + 1);
            scheduleAt(simTime(), ins);
        }
        if (std::all_of(layer_done.cbegin(), layer_done.cend(), [](bool done) {
            return done;
        })) {
            // all layers done, iter ends
            EV_DEBUG
                            << fmt::format(
                                    "Worker {} Rank {} Job {} done iter {} layer {} ",
                                    wid, rank, jid, iter, layer) << simTime()
                            << endl;
            iter++;
            std::fill(layer_done.begin(), layer_done.end(), false);
            auto iter_time = simTime() - iter_start.front();
            iter_start.pop();
            emit(fullIterTime, iter_time);
            auto comm_time = iter_time - all_fps_and_last_bp_time
                    - wu_times[layer];
            emit(commTime, comm_time);
            simtime_t real_comm_sum = 0;
            for (const auto &t : real_comm_times) {
                real_comm_sum += t;
            }
            std::fill(real_comm_times.begin(), real_comm_times.end(), 0);
            emit(realCommTime, real_comm_sum);
//            emit(delayTime, comm_time - real_comm_sum);
            if (iter == iters) {
                EV_DEBUG
                                << fmt::format(
                                        "Worker {} Rank {} Job {} iter {} done job {} ",
                                        wid, rank, jid, iter, jid) << simTime()
                                << endl;
                job->setFinish_time(simTime());
                job->setKind(5);
                sendDirect(job, getParentModule(), "directin");
            }
        }
        delete req;
        break;
    }
    case 8: {
        // CollectiveOperationRequest, partial completion (chunks)
        auto req = (CollectiveOperationRequest*) msg;
        real_comm_times[req->getLayer()] += (simTime() - req->getStart());
        delete req;
        break;
    }
    default:

        EV_FATAL << "got unexpected message " << msg->getKind() << endl;
        delete msg;
        break;
    }
}

void TrainingProcess::finish() {
}

TrainingProcess::~TrainingProcess() {
}
