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
    minIdleTime = registerSignal("minIdleTime");
    minIdleTimeWu = registerSignal("minIdleTimeWu");
    commTime = registerSignal("commTime");
    realCommTime = registerSignal("realCommTime");
    workerJobCompletionTime = registerSignal("workerJobCompletionTime");
    datarate = // Gbps
            uint64_t(
                    getParentModule()->gate("port$o")->getChannel()->par(
                            "datarate").doubleValue()/* bps */) / 1000000000;
}

void TrainingProcess::startComm(uint64_t layer, uint64_t iter) {
    if (distributed) {
        EV_DEBUG
                        << fmt::format(
                                "Worker {} Rank {} Job {} iter {} start comm layer {} ",
                                wid, rank, jid, iter, layer) << simTime()
                        << endl;
        auto size = model_sizes[model][layer];
        auto req = new CollectiveOperationRequest();
        req->setKind(0);
        req->setTraining_process_id(getId());
        req->setWorker_id(wid);
        req->setModel(job->getModel());
        req->setSize(size);
        req->setRank(rank);
        TensorKey tensor_key;
        tensor_key.job_id = jid;
        tensor_key.layer = layer;
        req->setTensor_key(tensor_key);
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
        TensorKey tensor_key;
        tensor_key.layer = layer;
        tensor_key.job_id = jid;
        req->setTensor_key(tensor_key);
        scheduleAfter(wu_times[model][layer], req);
        EV_DEBUG
                        << fmt::format(
                                "Worker {} Rank {} Job {} iter {} start wu layer {} ",
                                wid, rank, jid, iter, layer) << simTime()
                        << endl;
        recordIdleTimeIfAny();
    }
}

void insert_int(std::vector<int64_t> &vec, const char *s, char delim = ',') {
    std::stringstream ss(s);
    std::string item;
    while (getline(ss, item, delim)) {
        vec.push_back(std::stoll(item));
    }
}

void insert_simtime(std::vector<simtime_t> &vec, const char *s,
        char delim = ',') {
    std::stringstream ss(s);
    std::string item;
    while (getline(ss, item, delim)) {
        vec.push_back(SimTime(std::stoull(item), SIMTIME_PS));
    }
}

void TrainingProcess::markIdleStart() {
    gpu_start_idle_time = simTime();
}

void TrainingProcess::recordIdleTimeIfAny() {
    if (!gpu_start_idle_time.isZero()) {
        emit(idleTimeWu, simTime() - gpu_start_idle_time);
        gpu_start_idle_time = 0;
    }
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
            model = num_models - 1; // last index referrs to the custom model
            if (model_sizes[model].empty()) {
                insert_int(model_sizes[model],
                        job_dispatcher->par("custom_model_sizes").stringValue());
                insert_simtime(fp_times[model],
                        job_dispatcher->par("custom_fp_times").stringValue());
                insert_simtime(bp_times[model],
                        job_dispatcher->par("custom_bp_times").stringValue());
                insert_simtime(wu_times[model],
                        job_dispatcher->par("custom_wu_times").stringValue());
                n_layers[model] = model_sizes[model].size();
                fp_times[model].resize(n_layers[model]);
                bp_times[model].resize(n_layers[model]);
                wu_times[model].resize(n_layers[model]);
                for (const auto &fp : fp_times[model]) {
                    all_fps_and_last_bp_times[model] += fp;
                    all_fps_and_bps_times[model] += fp;
                }
                for (const auto &bp : bp_times[model]) {
                    all_fps_and_bps_times[model] += bp;
                }
                all_fps_and_last_bp_times[model] +=
                        bp_times[model][bp_times[model].size() - 1];
                min_wait_times[model] = min_wait_time(model, false, datarate);
                min_wait_times_wu[model] = min_wait_time(model, true, datarate);
                for (const auto &mwt : min_wait_times[model]) {
                    min_wait_time_sums[model] += mwt;
                }
                for (const auto &mwt : min_wait_times_wu[model]) {
                    min_wait_time_sums_wu[model] += mwt;
                }
            }
            uint64_t custom_iters = job_dispatcher->par("custom_iters");
            if (custom_iters) {
                iters = custom_iters;
            }
        } else {
            model = job->getModel();
            if (fp_times[model].empty()) {
                insert_simtime(fp_times[model], fp_times_raw[model]);
                insert_simtime(bp_times[model], bp_times_raw[model]);
                insert_simtime(wu_times[model], wu_times_raw[model]);
                all_fps_and_last_bp_times[model] = all_fps_and_last_bp(model);
                all_fps_and_bps_times[model] = all_fps_and_bps(model);
                min_wait_times[model] = min_wait_time(model, false, datarate);
                min_wait_times_wu[model] = min_wait_time(model, true, datarate);
                for (const auto &mwt : min_wait_times[model]) {
                    min_wait_time_sums[model] += mwt;
                }
                for (const auto &mwt : min_wait_times_wu[model]) {
                    min_wait_time_sums_wu[model] += mwt;
                }
            }
        }
        can_do_fp.resize(n_layers[model], 1); // for first iter, only require one fp ack ("wu" already done)
        layer_done.resize(n_layers[model], false);
        real_comm_times.resize(n_layers[model], 0);
        EV_DEBUG
                        << fmt::format(
                                "Start {}Job {} as rank {} iters {} num_layers {}",
                                distributed ? "distributed " : "", jid, rank,
                                iters, n_layers[model]) << endl;
        auto ins = new Instruction("fp", 20);
        scheduleAt(simTime(), ins);
        break;
    }
    case 20: {
        auto ins = (Instruction*) msg;
        auto layer = ins->getLayer();
        auto insIter = ins->getIter();
        // FP layer "layer-1" (or BP layer 0 if layer==0) done, try to start layer "layer"
        if (layer == 0 && insIter > 0) {
            // last bp layer
            EV_DEBUG
                            << fmt::format(
                                    "Worker {} Rank {} Job {} iter {} done bp layer {} ",
                                    wid, rank, jid, insIter, layer) << simTime()
                            << endl;
            markIdleStart();
            if (insIter == iters) {
                // last bp is done, only last comm and wu left for this job
                delete ins;
                break;
            }
        }
        can_do_fp[layer] += 1;

        if (can_do_fp[layer] < 2) {
            EV_DEBUG << "can't do fp " << layer << " iter " << insIter << endl;
            delete ins;
            break;
        } else {
            EV_DEBUG << "can do fp " << layer << " iter " << insIter << endl;
        }
        can_do_fp[layer] = 0;

        auto next_layer = layer + 1;
        if (layer == 0) {
            EV_DEBUG
                            << fmt::format(
                                    "Worker {} Rank {} Job {} start iter {} ",
                                    wid, rank, jid, insIter) << simTime()
                            << endl;

            iter_start.push(simTime());
        } else {
            EV_DEBUG
                            << fmt::format(
                                    "Worker {} Rank {} Job {} iter {} done fp layer {} ",
                                    wid, rank, jid, insIter, layer - 1)
                            << simTime() << endl;
            markIdleStart();
        }

        if (next_layer == n_layers[model]) {
            // last fp, proceed to bp
            ins->setKind(21);
        } else {
            ins->setLayer(next_layer);
        }
        scheduleAfter(fp_times[model][layer], ins);
        EV_DEBUG
                        << fmt::format(
                                "Worker {} Rank {} Job {} iter {} start fp layer {} ",
                                wid, rank, jid, insIter, layer) << simTime()
                        << endl;
        recordIdleTimeIfAny();
        break;
    }
    case 21: {
        auto ins = (Instruction*) msg;
        auto layer = ins->getLayer();
        bool first_bp = layer == n_layers[model] - 1;
        if (first_bp) {
            // just done last fp layer
            EV_DEBUG
                            << fmt::format(
                                    "Worker {} Rank {} Job {} iter {} done fp layer {} ",
                                    wid, rank, jid, iter, layer) << simTime()
                            << endl;
            markIdleStart();
        } else {
            // just done a bp layer
            EV_DEBUG
                            << fmt::format(
                                    "Worker {} Rank {} Job {} iter {} done bp layer {} ",
                                    wid, rank, jid, iter, layer + 1)
                            << simTime() << endl;
            markIdleStart();
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
            scheduleAfter(bp_times[model][layer], ack);
        } else {
            ins->setLayer(layer - 1);
        }
        scheduleAfter(bp_times[model][layer], ins);
        EV_DEBUG
                        << fmt::format(
                                "Worker {} Rank {} Job {} iter {} start bp layer {} ",
                                wid, rank, jid, iter, layer) << simTime()
                        << endl;
        recordIdleTimeIfAny();
        break;
    }
    case 22: {
        // last bp (layer 0)
        auto ins = (Instruction*) msg;
        auto insIter = ins->getIter();
        EV_DEBUG
                        << fmt::format(
                                "Worker {} Rank {} Job {} iter {} done bp layer {} ",
                                wid, rank, jid, insIter, 0) << simTime()
                        << endl;
        markIdleStart();
        startComm(0, insIter);
        delete ins;
        break;
    }
    case 2: {
        // CollectiveOperationRequest meaning collective is done, schdule after wu
        msg->setKind(4);
        auto req = (CollectiveOperationRequest*) msg;
        auto layer = req->getTensor_key().layer;
        scheduleAfter(wu_times[model][layer], req);

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
        recordIdleTimeIfAny();
        break;
    }
    case 4: {
        auto req = (CollectiveOperationRequest*) msg;
        auto layer = req->getTensor_key().layer;
        EV_DEBUG
                        << fmt::format(
                                "Worker {} Rank {} Job {} iter {} done wu layer {} ",
                                wid, rank, jid, iter, layer) << simTime()
                        << endl;
        markIdleStart();
        if (layer == 0) {
            // time when next iter fp will start
            auto contiguous_iter_time = simTime() - iter_start.front();
            emit(contiguousIterTime, contiguous_iter_time);
            emit(idleTime, contiguous_iter_time - all_fps_and_bps_times[model]);
            emit(minIdleTime, min_wait_time_sums[model]);
            emit(minIdleTimeWu, min_wait_time_sums_wu[model]);
        }
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
            auto iter_comm_time = iter_time - all_fps_and_last_bp_times[model]
                    - wu_times[model][layer];
            emit(commTime, iter_comm_time);
            simtime_t real_comm_sum = 0;
            for (const auto &t : real_comm_times) {
                real_comm_sum += t;
            }
            emit(realCommTime, real_comm_sum);
            std::fill(real_comm_times.begin(), real_comm_times.end(), 0);
//            emit(delayTime, comm_time - real_comm_sum);
            if (iter == iters) {
                EV_DEBUG
                                << fmt::format(
                                        "Worker {} Rank {} Job {} iter {} done job {} ",
                                        wid, rank, jid, iter, jid) << simTime()
                                << endl;

                job->setFinish_time(simTime());
                job->setKind(5);
                emit(workerJobCompletionTime, simTime() - job->getStart_time());
                sendDirect(job, getParentModule(), "directin");
            }
        }
        delete req;
        break;
    }
    case 8: {
        // CollectiveOperationRequest, partial completion (chunks)
        auto req = (CollectiveOperationRequest*) msg;
        auto layer = req->getTensor_key().layer;
        real_comm_times[layer] += (simTime() - req->getStart());
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
