#include "TrainingProcess.h"
#include "ModelStats.h"
#include "Worker.h"

Define_Module(TrainingProcess);

void TrainingProcess::allreduce(Job *job, uint64_t layer, uint64_t size,
        uint64_t iter) {
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

void TrainingProcess::process_ack(LayerAck *ack) {
    can_do_fp[ack->getLayer()] = true;
    delete ack;
}

void TrainingProcess::waitAndProcessAck(simtime_t wait_time, cQueue *AckQueue) {
    waitAndEnqueue(wait_time, AckQueue);
    while (!AckQueue->isEmpty()) {
        process_ack(check_and_cast<LayerAck*>(AckQueue->pop()));
    }
}

void TrainingProcess::activity() {
    // retrieve parameters
    collective_scheduler = getSimulation()->findModuleByPath(
            "<root>.collective_scheduler");
    worker = (Worker*) getParentModule();
    if (collective_scheduler) {
        EV_DEBUG << "Collective Scheduler is "
                        << collective_scheduler->getFullName() << endl;
    } else
        EV_DEBUG << "No Collective Scheduler" << endl;
    job = (Job*) (receive()); // from worker
    auto rank = job->getRank();
    auto jid = job->getJob_id();
    auto wid = worker->getId();
    auto iters = job->getIters();

    auto model = job->getModel();
    auto num_layers = n_layers(model);

    bool distributed = job->getNum_workers_allocated() > 1;
    can_do_fp.resize(num_layers, true);
    EV_DEBUG
                    << fmt::format(
                            "Start {}Job {} as rank {} iters {} num_layers {}",
                            distributed ? "distributed " : "", jid, rank, iters,
                            num_layers) << endl;
    cQueue AckQueue(fmt::format("Allreducer{}", getId()).c_str());

    for (unsigned iter = 0; iter < iters; ++iter) {
        for (size_t layer = 0; layer < num_layers; ++layer) {
            while (!can_do_fp[layer]) {
                process_ack(check_and_cast<LayerAck*>(receive()));
            }
            waitAndProcessAck(SimTime(fp_times[model][layer], SIMTIME_PS),
                    &AckQueue);
            EV_DEBUG
                            << fmt::format(
                                    "Worker {} Job {} iter {} done fp layer {}\n",
                                    wid, jid, iter, layer);
            can_do_fp[layer] = false;
        }

        for (int layer = num_layers - 1; layer >= 0; --layer) {
            waitAndProcessAck(SimTime(bp_times[model][layer], SIMTIME_PS),
                    &AckQueue);
            EV_DEBUG
                            << fmt::format(
                                    "Worker {} Job {} iter {} done bp layer {}\n",
                                    wid, jid, iter, layer);
            if (distributed) {
                allreduce(job, layer,
                        model_sizes[model][layer] < 0 ?
                                worker->par("test_tensor_size") :
                                model_sizes[model][layer], iter);
            } else {
                auto ack = new LayerAck();
                ack->setLayer(layer);
                scheduleAfter(SimTime(wu_times[model][layer], SIMTIME_PS), ack);
            }
        }
    }

    for (size_t i = 0; i < num_layers; ++i) {
        while (!can_do_fp[i]) {
            process_ack(check_and_cast<LayerAck*>(receive()));
        }
    }

    EV_DEBUG << "rank " << rank << " done job " << jid << " at " << simTime()
                    << endl;
    job->setFinish_time(simTime());
    job->setKind(5);
    sendDirect(job, getParentModule(), "directin");
    deleteModule();
}

TrainingProcess::~TrainingProcess() {
}
