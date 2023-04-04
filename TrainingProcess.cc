#include "TrainingProcess.h"
#include "ModelStats.h"
#include <sstream>
#include <vector>

Define_Module(TrainingProcess);

void TrainingProcess::initialize() {
    compress_probability = par("compress_probability");
    fullIterTime = registerSignal("fullIterTime");
    contiguousIterTime = registerSignal("contiguousIterTime");
    idleTime = registerSignal("idleTime");
    idleTimeWu = registerSignal("idleTimeWu");
//    minIdleTime = registerSignal("minIdleTime");
//    minIdleTimeWu = registerSignal("minIdleTimeWu");
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
        comm_start_times[layer] = simTime();
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
            if (print)
                EV_DETAIL << "[TrainingProcess]\t" << simTime()
                                 << fmt::format(
                                         "\tWorker {} Rank {} Job {} iter {} enqueue collective operation for layer {} size {} ",
                                         wid, rank, jid, iter, layer, size)
                                 << " at " << simTime() << endl;
            sendDirect(req, collective_scheduler, "directin");
        } else if (compress_probability > 0
                && uniform(0, 1) < compress_probability) {
            req->setKind(17);
            EV_DETAIL << "[CollectiveScheduler]\t" << simTime()
                             << fmt::format(
                                     "\tNone compress Worker {} Job {} layer {}, size {}",
                                     req->getWorker_id(), jid, layer,
                                     req->getSize()) << endl;
            sendDirect(req, getParentModule(), "directin");
        } else { // send directly to Worker
            sendDirect(req, getParentModule(), "directin");
        }

    } else {
        auto req = new CollectiveOperationRequest("req", 4);
        TensorKey tensor_key;
        tensor_key.layer = layer;
        tensor_key.job_id = jid;
        req->setTensor_key(tensor_key);
        auto wu_time = normal(wu_times[model][layer],
                wu_times[model][layer] / 50);
        scheduleAfter(wu_time, req);
        if (print)
            EV_DETAIL << "[TrainingProcess]\t" << simTime()
                             << fmt::format(
                                     "\tWorker {} Rank {} Job {} iter {} start wu layer {} duration ",
                                     wid, rank, jid, iter, layer) << wu_time
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
    if (print)
        EV_DEBUG << "[TrainingProcess]\t" << simTime() << "\tstart idleTimeWu "
                        << simTime() << endl;
}

void TrainingProcess::recordIdleTimeIfAny() {
    if (!gpu_start_idle_time.isZero() && simTime() > gpu_start_idle_time) {
        emit(idleTimeWu, simTime() - gpu_start_idle_time);
        if (print)
            EV_DEBUG << "[TrainingProcess]\t" << simTime()
                            << "\trecord idleTimeWu " << simTime() << " - "
                            << gpu_start_idle_time << endl;
        gpu_start_idle_time = SimTime::ZERO;
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
        rank = job->getRank();
        print = !(par("print_only_rank0").boolValue() && rank);
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
                all_fps_and_last_bp_times[model] += bp_times[model].back();
                for (const auto &bp : bp_times[model]) {
                    all_fps_and_bps_times[model] += bp;
                }
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
                if (worker->par("gpu_type").stdstringValue() == "v100") {
                    insert_simtime(fp_times[model], fp_times_raw_v100[model]);
                    insert_simtime(bp_times[model], bp_times_raw_v100[model]);
                    insert_simtime(wu_times[model], wu_times_raw_v100[model]);
                } else if (worker->par("gpu_type").stdstringValue() == "a100") {
                    insert_simtime(fp_times[model], fp_times_raw_a100[model]);
                    insert_simtime(bp_times[model], bp_times_raw_a100[model]);
                    insert_simtime(wu_times[model], wu_times_raw_a100[model]);
                } else if (worker->par("gpu_type").stdstringValue()
                        == "a100_match_v100_bs") {
                    insert_simtime(fp_times[model],
                            fp_times_raw_a100_match_v100_bs[model]);
                    insert_simtime(bp_times[model],
                            bp_times_raw_a100_match_v100_bs[model]);
                    insert_simtime(wu_times[model],
                            wu_times_raw_a100_match_v100_bs[model]);
                }
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
        comm_start_times.resize(n_layers[model], 0);
        if (print)
            EV_DETAIL << "[TrainingProcess]\t" << simTime()
                             << fmt::format(
                                     "\tStart {}Job {} as rank {} iters {} num_layers {}",
                                     distributed ? "distributed " : "", jid,
                                     rank, iters, n_layers[model]) << endl;
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
            markIdleStart();
            if (insIter == iters) {
                // last bp is done, only last comm and wu left for this job
                delete ins;
                break;
            }
        }
        can_do_fp[layer] += 1;

        if (can_do_fp[layer] < 2) {
//            if (print) EV_DEBUG << "[TrainingProcess]\t" << simTime() << "\tcan't do fp "
//                            << layer << " iter " << insIter << endl;
            delete ins;
            break;
//        } else {
//            if (print) EV_DEBUG << "[TrainingProcess]\t" << simTime() << "\tcan do fp "
//                            << layer << " iter " << insIter << endl;
        }
        can_do_fp[layer] = 0;

        auto next_layer = layer + 1;
        if (layer == 0) {
            iter_start.push(simTime());
        } else {
            markIdleStart();
        }

        if (next_layer == n_layers[model]) {
            // last fp, proceed to bp
            ins->setKind(21);
        } else {
            ins->setLayer(next_layer);
        }
        auto fp_time = normal(fp_times[model][layer],
                fp_times[model][layer] / 50);
        scheduleAfter(fp_time, ins);
        if (print) {
            EV_DETAIL << "[TrainingProcess]\t" << simTime()
                             << fmt::format(
                                     "\tWorker {} Rank {} Job {} iter {} start fp layer {} duration ",
                                     wid, rank, jid, insIter, layer) << fp_time
                             << endl;
        }
        recordIdleTimeIfAny();
        break;
    }
    case 21: {
        auto ins = (Instruction*) msg;
        auto layer = ins->getLayer();
        bool first_bp = layer == n_layers[model] - 1;
        if (first_bp) {
            // just done last fp layer
            markIdleStart();
        } else {
            // just done a bp layer
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
            scheduleAfter(
                    normal(bp_times[model][layer], bp_times[model][layer] / 50),
                    ack);
        } else {
            ins->setLayer(layer - 1);
        }
        auto bp_time = normal(bp_times[model][layer],
                bp_times[model][layer] / 50);
        scheduleAfter(bp_time, ins);
        if (print)
            EV_DETAIL << "[TrainingProcess]\t" << simTime()
                             << fmt::format(
                                     "\tWorker {} Rank {} Job {} iter {} start bp layer {} duration ",
                                     wid, rank, jid, iter, layer) << bp_time
                             << endl;
        recordIdleTimeIfAny();
        break;
    }
    case 22: {
        // last bp (layer 0)
        auto ins = (Instruction*) msg;
        auto insIter = ins->getIter();
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
        auto wu_time = normal(wu_times[model][layer],
                wu_times[model][layer] / 50);
        scheduleAfter(wu_time, req);
        if (print)
            EV_DEBUG << "done collective for " << simTime() - req->getStart()
                            << endl;

        real_comm_times[layer] += (simTime() - req->getStart());
        if (print) {
            EV_DETAIL << "[TrainingProcess]\t" << req->getStart()
                             << fmt::format(
                                     "\tWorker {} Rank {} Job {} iter {} start chunk {} layer {} duration ",
                                     wid, rank, jid, iter, req->getChunk_id(),
                                     layer) << simTime() - req->getStart()
                             << endl;
            // done comm
            EV_DETAIL << "[TrainingProcess]\t" << comm_start_times[layer]
                             << fmt::format(
                                     "\tWorker {} Rank {} Job {} iter {} start comm layer {} duration ",
                                     wid, rank, jid, iter, layer)
                             << simTime() - comm_start_times[layer] << endl;

            EV_DETAIL << "[TrainingProcess]\t" << simTime()
                             << fmt::format(
                                     "\tWorker {} Rank {} Job {} iter {} start wu layer {} duration ",
                                     wid, rank, jid, iter, layer) << wu_time
                             << endl;
        }
        recordIdleTimeIfAny();
        break;
    }
    case 4: {
        auto req = (CollectiveOperationRequest*) msg;
        auto layer = req->getTensor_key().layer;
        markIdleStart();
        if (layer == 0) {
            // time when next iter fp will start
            auto contiguous_iter_time = simTime() - iter_start.front();
            emit(contiguousIterTime, contiguous_iter_time);
            emit(idleTime, contiguous_iter_time - all_fps_and_bps_times[model]);
            if (print)
                EV_DEBUG << "[TrainingProcess]\t" << simTime()
                                << "\trecord idleTime " << contiguous_iter_time
                                << " - " << all_fps_and_bps_times[model]
                                << endl;
//            emit(minIdleTime, min_wait_time_sums[model]);
//            emit(minIdleTimeWu, min_wait_time_sums_wu[model]);
        }
        layer_done[layer] = true;
        if (iter + 1 < iters) {
            // start next iteration
            auto ins = new Instruction("fp", 20);
            ins->setLayer(layer);
            ins->setIter(iter + 1);
            scheduleAt(simTime(), ins);
        } else if (layer == 0) {
            // mark simtime for the final comm and wu times
            last_idle_times_start = simTime();
        }
        if (std::all_of(layer_done.cbegin(), layer_done.cend(), [](bool done) {
            return done;
        })) {
            // all wu layers done, iter ends
            if (print) {
                EV_DETAIL << "[TrainingProcess]\t" << iter_start.front()
                                 << fmt::format(
                                         "\tWorker {} Rank {} Job {} iter {} start iter layer {} duration ",
                                         wid, rank, jid, iter, layer)
                                 << simTime() - iter_start.front() << endl;
            }
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
                if (print)
                    EV_DETAIL << "[TrainingProcess]\t" << simTime()
                                     << fmt::format(
                                             "\tWorker {} Rank {} Job {} iter {} done job {}",
                                             wid, rank, jid, iter, jid) << endl;

                job->setFinish_time(simTime());
                job->setKind(5);
                emit(workerJobCompletionTime, simTime() - job->getStart_time());

                // last idle times includes comm and wus (for 1+ layers) for last iter
                if (simTime() > last_idle_times_start) {
                    emit(idleTime, simTime() - last_idle_times_start);
                    if (print)
                        EV_DEBUG << "[TrainingProcess]\t" << simTime()
                                        << "\trecord idleTime " << simTime()
                                        << " - " << last_idle_times_start
                                        << endl;
                }
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
        if (print) {
            EV_DETAIL << "[TrainingProcess]\t" << req->getStart()
                             << fmt::format(
                                     "\tWorker {} Rank {} Job {} iter {} start chunk {} layer {} duration ",
                                     wid, rank, jid, iter, req->getChunk_id(),
                                     layer) << simTime() - req->getStart()
                             << endl;
        }
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
