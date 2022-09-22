#include "TrainingProcess.h"
#include "ModelStats.h"
#include <sstream>
#include <vector>

Define_Module(TrainingProcess);

void TrainingProcess::initialize() {

}

void TrainingProcess::allreduce(uint64_t layer, uint64_t iter) {
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
            auto m = job->getModel();
            num_layers = n_layers(m);
            model.reserve(num_layers);
            fp_times.reserve(num_layers);
            bp_times.reserve(num_layers);
            wu_times.reserve(num_layers);
            for (size_t layer = 0; layer < num_layers; ++layer) {
                model.push_back(model_size(m, layer));
                fp_times.push_back(SimTime(fp_time(m, layer), SIMTIME_PS));
                bp_times.push_back(SimTime(bp_time(m, layer), SIMTIME_PS));
                wu_times.push_back(SimTime(wu_time(m, layer), SIMTIME_PS));
            }
        }

        can_do_fp.resize(num_layers, true);
        layer_done.resize(num_layers, false);

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
        // FP layer "layer-1" (or BP layer 0 if layer==0) done, try to start layer "layer"
        if (!can_do_fp[layer]) {
            // waiting for communication, so idling
            idle_start.push(simTime());
            delete ins;
            break;
        } else if (!idle_start.empty()) {
            // record idle time
        }
        can_do_fp[layer] = false; // will be reset until comm and wu for this layer are done
        if (layer == 0) {
            iter_start.push(simTime());
        } else {
            EV_DEBUG
                            << fmt::format(
                                    "Worker {} Job {} iter {} done fp layer {} ",
                                    wid, jid, iter, layer - 1) << simTime()
                            << endl;
        }
        ins->setLayer(layer + 1);
        if (layer + 1 == num_layers) {
            // next is bp
            ins->setKind(21);
        }
        scheduleAfter(fp_times[layer], ins);
//            EV_DEBUG << "start fp layer " << layer << " / " << num_layers << " "
//                                        << fp_times[layer] << " at " << simTime() << endl;
        break;
    }
    case 21: {
        auto ins = (Instruction*) msg;
        auto layer = ins->getLayer();
        if (layer < num_layers) {
            EV_DEBUG
                            << fmt::format(
                                    "Worker {} Job {} iter {} done bp layer {} ",
                                    wid, jid, iter, layer) << simTime() << endl;
            // start or enqueue collective of layer "layer"
            if (distributed) {
                if (layer == num_layers - 1) {
                    comm_start.push(simTime());
                }
//                    EV_DEBUG << "start comm layer " << layer << " at "
//                                    << simTime() << endl;
                allreduce(layer, ins->getIter());
            } else {
                auto ack = new LayerAck("ack", 4);
                ack->setLayer(layer);
                scheduleAfter(wu_times[layer], ack);
//                    EV_DEBUG << "start wu layer " << layer << " "
//                                    << wu_times[layer] << endl;
            }
        } else {
            EV_DEBUG
                            << fmt::format(
                                    "Worker {} Job {} iter {} done fp layer {} ",
                                    wid, jid, iter, layer - 1) << simTime()
                            << endl;
        }
        if (layer == 0) {
            // next iter layer 0
            auto next_iter = ins->getIter() + 1;
            if (next_iter < iters) {
                ins->setKind(20);
                ins->setIter(next_iter);
                scheduleAt(simTime(), ins);
            } else {
                delete ins;
            }
        } else {
            ins->setLayer(layer - 1);
            scheduleAfter(bp_times[layer - 1], msg);
//                EV_DEBUG << "start bp layer " << layer - 1 << " "
//                                << bp_times[layer - 1] << " at " << simTime()
//                                << endl;
        }
        break;
    }
    case 2: {
        // LayerAck meaning collective is done, schdule after wu
        msg->setKind(4);
        auto ack = (LayerAck*) msg;
        scheduleAfter(wu_times[ack->getLayer()], msg);
        //                    EV_DEBUG << "done comm layer " << layer << " at "
        //                                    << simTime() << endl;
        //                    EV_DEBUG << "start wu layer " << layer << " "
        //                                    << wu_times[layer] << endl;
        break;
    }
    case 4: {
        auto ack = (LayerAck*) msg;
        //                    EV_DEBUG << "done wu layer " << layer << " "
        //                                    << wu_times[layer] << endl;
        can_do_fp[ack->getLayer()] = true;
        layer_done[ack->getLayer()] = true;
        if (iter + 1 < iters) {
            auto ins = new Instruction("fp", 20);
            ins->setIter(iter + 1);
            scheduleAt(simTime(), ins);
        }
        delete ack;
        if (std::all_of(layer_done.cbegin(), layer_done.cend(), [](bool done) {
            return done;
        })) {
            // iter end time
            iter++;
            std::fill(layer_done.begin(), layer_done.end(), false);
            if (iter == iters) {
                EV_DEBUG << "rank " << rank << " done job " << jid << " at "
                                << simTime() << endl;
                job->setFinish_time(simTime());
                job->setKind(5);
                sendDirect(job, getParentModule(), "directin");
            }
        }
        break;
    }
    case 8: {
        // partial completion, chunks
        delete msg;
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
