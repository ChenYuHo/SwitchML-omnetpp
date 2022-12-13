#include "SwitchML_m.h"
#include "JobDispatcher.h"
#include "ModelStats.h"
#include "TrainingProcess.h"
#include <unordered_map>
#include <queue>
#define FMT_HEADER_ONLY
#include "fmt/format.h"
using namespace omnetpp;

class Sincronia: public cSimpleModule {
private:
    uint64_t chunk_size;
    std::unordered_map<TensorKey, uint64_t> remaining_sizes { };
    std::unordered_map<TensorKey, std::vector<CollectiveOperationRequest*>> requests_of_key { };
    std::unordered_map<uint64_t, std::priority_queue<TensorKey>> queues_for_job { };
    std::unordered_map<uint64_t, short> model_for_jid { };
//    std::unordered_map<uint64_t, bool> busy { };
    JobDispatcher *job_dispatcher { };
    void clean_resources_for_job(uint64_t);
    void clean_resources_for_tensor(const TensorKey&);
    void initialize() override;
    void handleMessage(cMessage *msg) override;
//    void order_and_run();
    void updatePendingTensors();
    double get_weight(const TensorKey&);
    unsigned StartCollectiveOperations();
    std::deque<TensorKey> pending_tensors { };
    std::unordered_map<TensorKey, unsigned> num_workers_of_active_tensor_key { };
    bool exclusive;
};

Define_Module(Sincronia);

void Sincronia::initialize() {
    exclusive = par("exclusive");
    chunk_size = par("chunk_size");
    job_dispatcher = (JobDispatcher*) getModuleByPath("^.job_dispatcher");
}

void Sincronia::clean_resources_for_tensor(const TensorKey &tensor_key) {
    for (auto &req : requests_of_key[tensor_key]) {
        delete req;
    }
    requests_of_key.erase(tensor_key);
    remaining_sizes.erase(tensor_key);
}

void Sincronia::clean_resources_for_job(uint64_t jid) {
    queues_for_job.erase(jid);
//    busy.erase(jid);

    for (auto iterator = num_workers_of_active_tensor_key.begin();
            iterator != num_workers_of_active_tensor_key.end();) {
        if (iterator->first.job_id == jid) {
            iterator = num_workers_of_active_tensor_key.erase(iterator);
        } else {
            ++iterator;
        }
    }
}

double Sincronia::get_weight(const TensorKey &tensor_key) {
    // remaining size
    auto weighting_fn = this->par("weighting_fn").stdstringValue();
    if (weighting_fn == "remaining_sizes_more") {
        // the more remaining, the higher priority
        return double(remaining_sizes[tensor_key])
                / double(
                        model_sizes[model_for_jid[tensor_key.job_id]][tensor_key.layer]);
    } else if (weighting_fn == "remaining_sizes_less") {
        // the less remaining, the higher priority
        return 1.
                - double(remaining_sizes[tensor_key])
                        / double(
                                model_sizes[model_for_jid[tensor_key.job_id]][tensor_key.layer]);
    } else if (weighting_fn == "layer") {
        // front layers get higher priority
        return 1.
                - double(tensor_key.layer)
                        / double(n_layers[model_for_jid[tensor_key.job_id]]);
    } else if (weighting_fn == "idle") {
        auto t = SimTime::ZERO;
        for (auto req : requests_of_key[tensor_key]) {
            auto tp = (TrainingProcess*) (req->getSenderModule());
            if (!tp->gpu_start_idle_time.isZero()
                    && simTime() > tp->gpu_start_idle_time) {
                t += simTime() - tp->gpu_start_idle_time;
            }
        }
        return t.dbl();
    } else { // none
        return 1.;
    }
}

unsigned Sincronia::StartCollectiveOperations() {
    if (pending_tensors.empty())
        return 0;
    unsigned started = 0;
//    auto last_size = (uint64_t) -1; // 2^64-1, largest uint64_t
    int priority = 1; // the smaller, the higher priority
    for (auto iterator = pending_tensors.begin();
            iterator != pending_tensors.end();) {
        auto &tensor_key = *iterator;
        auto &requests = requests_of_key[tensor_key];
        auto jid_to_add = tensor_key.job_id;
        auto layer = tensor_key.layer;
        if (num_workers_of_active_tensor_key.find(tensor_key)
                != num_workers_of_active_tensor_key.end()) {
            // already invoked, just update priority
            for (auto &req : requests) {
                EV_DEBUG
                                << fmt::format(
                                        "Sincronia notifies Worker {} to update priority for Collective Operation Job {} layer {}, size {} priority {}\n",
                                        req->getWorker_id(), jid_to_add, layer,
                                        req->getSize(), priority);
                req->setPriority(priority);
                auto update = req->dup();
                update->setKind(14);
                sendDirect(update,
                        getSimulation()->getModule(req->getWorker_id()),
                        "directin");
            }
        } else {
            // add to active
            started++;
            auto next_chunk_id = requests[0]->getChunk_id() + 1;
            bool last_chunk = next_chunk_id == requests[0]->getNum_chunks();
            for (auto &req : requests) {
                if (last_chunk) {
                    req->setSize(remaining_sizes[tensor_key]);
                }
                EV_DEBUG
                                << fmt::format(
                                        "Sincronia notifies Worker {} to start Collective Operation for Job {} layer {}, chunk {}/{} size {} priority {}\n",
                                        req->getWorker_id(), jid_to_add, layer,
                                        next_chunk_id, req->getNum_chunks(),
                                        req->getSize(), priority);
                req->setPriority(priority);
                sendDirect(req->dup(),
                        getSimulation()->getModule(req->getWorker_id()),
                        "directin");
                req->setChunk_id(next_chunk_id);
            }
            num_workers_of_active_tensor_key[tensor_key] = requests.size();
            if (last_chunk) {
                remaining_sizes[tensor_key] = 0;
            } else {
                remaining_sizes[tensor_key] -= chunk_size;
            }
        }
        ++iterator;
        ++priority;
//        if (job_dispatcher->accommodate(num_workers_of_active_tensor_key,
//                jid_to_add, exclusive)) {
//            auto this_size = std::min(remaining_sizes[tensor_key], chunk_size);
//            if (this_size <= last_size) {
//                // add to active
//                last_size = this_size; // to ensure strict ordering: work conservation doesn't take larger tensors
//                started++;
//                auto next_chunk_id = requests[0]->getChunk_id() + 1;
//                bool last_chunk = next_chunk_id == requests[0]->getNum_chunks();
//                for (auto &req : requests) {
//                    if (last_chunk) {
//                        req->setSize(remaining_sizes[tensor_key]);
//                    }
//                    EV_DEBUG
//                                    << fmt::format(
//                                            "Sincronia notifies Worker {} to start Collective Operation for Job {} layer {}, chunk {}/{} size {}\n",
//                                            req->getWorker_id(), jid_to_add,
//                                            layer, next_chunk_id,
//                                            req->getNum_chunks(),
//                                            req->getSize());
//                    sendDirect(req->dup(),
//                            getSimulation()->getModule(req->getWorker_id()),
//                            "directin");
//                    req->setChunk_id(next_chunk_id);
//                }
//                num_workers_of_active_tensor_key[tensor_key] = requests.size();
//                if (last_chunk) {
//                    remaining_sizes[tensor_key] = 0;
//                } else {
//                    remaining_sizes[tensor_key] -= chunk_size;
//                }
//                iterator = pending_tensors.erase(iterator);
//            } else {
//                ++iterator;
//            }
//        } else {
//            ++iterator;
//        }
    }

    return started;
}

//void Sincronia::order_and_run() {
//    if (!pending_tensors.empty())
//        return;
//
//    auto started = StartCollectiveOperations();
//}

void Sincronia::updatePendingTensors() {
    std::unordered_map<TensorKey, double> weights { }; // tensor_key -> weight
    for (auto &pair : queues_for_job) {
        auto &pq = pair.second;
        while (!pq.empty()) {
            auto &tensor_key = pq.top();
            if (remaining_sizes[tensor_key] == 0) {
                // this tensor is done!
                pq.pop();
                continue;
            }
            weights[tensor_key] = get_weight(tensor_key);
//            auto &req = requests_of_key[tensor_key][0];

//            EV_DEBUG << "Job " << tensor_key.job_id << " layer "
//                            << tensor_key.layer << " weight "
//                            << weights[tensor_key] << endl;
            break;// while loop
        }
    }
    // "running" tensors are in num_workers_of_active_tensor_key, so no worries
    pending_tensors.clear();
    if (weights.empty())
        return;
    // bssi
    if (weights.size() > 1) {
        job_dispatcher->bssi(pending_tensors, weights, remaining_sizes);
    } else {
        pending_tensors.push_back(weights.begin()->first);
    }
    EV_DEBUG << "after bssi:";
#ifndef NDEBUG
    for (auto &tkey : pending_tensors) {
        EV_DEBUG << " jid " << tkey.job_id << " layer " << tkey.layer;
    }
    EV_DEBUG << endl;
#endif
}

void Sincronia::handleMessage(cMessage *msg) {
    switch (msg->getKind()) {
    case 0: {
        // CollectiveOperationRequest from TrainingProcess
        auto request = (CollectiveOperationRequest*) (msg);
        auto &tensor_key = request->getTensor_key();
        auto &requests = requests_of_key[tensor_key];
        requests.push_back(request);
        if (requests.size() == request->getNum_workers_allocated()) {
            auto size = request->getSize();
            remaining_sizes[tensor_key] = size;
            auto num_chunks = size / chunk_size + (size % chunk_size ? 1 : 0);
            for (auto req : requests) {
                req->setSize(chunk_size);
                req->setNum_chunks(num_chunks);
            }
            // layers nearer the front (smaller index) gets higher priority
            auto jid = request->getTensor_key().job_id;
            EV_DEBUG << "Job " << jid << " layer " << tensor_key.layer
                            << " enqueued\n";
            queues_for_job[jid].push(tensor_key);
            model_for_jid[jid] = request->getModel();
            updatePendingTensors();
            StartCollectiveOperations();
        }
        break;
    }
    case 2: {
        // CollectiveOperationRequest from Worker, meaning a collective operation is done
        auto req = (CollectiveOperationRequest*) msg;
        auto &tensor_key = req->getTensor_key();
        auto jid = tensor_key.job_id;
        num_workers_of_active_tensor_key[tensor_key] -= 1;
        if (num_workers_of_active_tensor_key[tensor_key] == 0) {
            EV_DEBUG << "Job " << jid << " layer " << tensor_key.layer
                            << " done\n";
//            busy[jid] = false;
            auto &tensor_key = req->getTensor_key();
            if (remaining_sizes[tensor_key] == 0) {
                clean_resources_for_tensor(tensor_key);
            }
            num_workers_of_active_tensor_key.erase(tensor_key);
//            if (pending_tensors.empty()) {
            updatePendingTensors();
//            }
            StartCollectiveOperations();
        }
        delete msg;
        break;
    }
    case 5: {
        auto job = (Job*) msg;
        EV_DEBUG << "CollectiveScheduler cleans job resources for job "
                        << job->getJob_id() << endl;
        clean_resources_for_job(job->getJob_id());
        delete msg;
        break;
    }
    default:
        delete msg;
        EV_FATAL << "got unexpected message" << endl;
        break;
    }

}
