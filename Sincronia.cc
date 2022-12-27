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
    JobDispatcher *job_dispatcher { };
    void clean_resources_for_job(uint64_t);
    void clean_resources_for_tensor(const TensorKey&);
    void initialize() override;
    void handleMessage(cMessage *msg) override;
    void updatePendingTensors();
    double get_weight(const TensorKey&);
    unsigned StartCollectiveOperations();
    std::deque<TensorKey> pending_tensors { };
    std::unordered_map<TensorKey, unsigned> num_workers_of_active_tensor_key { };
    std::unordered_map<uint64_t, TensorKey> active_tensor_for_jid { };
    std::unordered_map<uint64_t, std::deque<TensorKey>> deferred_tensors { };
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
    active_tensor_for_jid.erase(jid);
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
    int priority = 1; // the smaller, the higher priority
    for (auto iterator = pending_tensors.begin();
            iterator != pending_tensors.end();) {
        auto &tensor_key = *iterator;
        auto &requests = requests_of_key[tensor_key];
        auto jid_to_add = tensor_key.job_id;
        auto layer = tensor_key.layer;
        if (active_tensor_for_jid.find(jid_to_add)
                != active_tensor_for_jid.end()) {
            // already running a chunk for jid_to_add, just update priority
            for (auto &req : requests) {
                EV_DETAIL << "[CollectiveScheduler]\t" << simTime()
                                 << fmt::format(
                                         "\tSincronia update priority Worker {} Job {} layer {}, size {} priority {}",
                                         req->getWorker_id(), jid_to_add, layer,
                                         req->getSize(), priority) << endl;
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
                } // else size is already set as chunk_size
                EV_DETAIL << "[CollectiveScheduler]\t" << simTime()
                                 << fmt::format(
                                         "\tSincronia start Collective Operation Worker {} Job {} layer {}, chunk {}/{} size {} priority {}",
                                         req->getWorker_id(), jid_to_add, layer,
                                         next_chunk_id, req->getNum_chunks(),
                                         req->getSize(), priority) << endl;
                req->setPriority(priority);
                sendDirect(req->dup(),
                        getSimulation()->getModule(req->getWorker_id()),
                        "directin");
                req->setChunk_id(next_chunk_id);
            }
            active_tensor_for_jid[tensor_key.job_id] = tensor_key;
            num_workers_of_active_tensor_key[tensor_key] = requests.size();
        }
        ++iterator;
        ++priority;
    }

    return started;
}

void Sincronia::updatePendingTensors() {
    std::unordered_map<TensorKey, double> weights { }; // tensor_key -> weight
    for (auto &pair : queues_for_job) {
        auto &pq = pair.second;
        while (!pq.empty()) {
            auto &tensor_key = pq.top();
            if (remaining_sizes[tensor_key] == 0) {
                // this tensor is done (or is running last chunk)!
                pq.pop();
                continue;
            }
            weights[tensor_key] = get_weight(tensor_key);
            break; // while loop
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
#ifndef NDEBUG
    EV_DEBUG << "after bssi:";
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
            auto jid = tensor_key.job_id;
            EV_DETAIL << "[CollectiveScheduler]\t" << simTime()
                             << fmt::format(
                                     "\tSincronia Job {} enqueue collective operation for layer {} size {} ",
                                     jid, tensor_key.layer, size) << endl;
            if (active_tensor_for_jid.find(jid)
                    == active_tensor_for_jid.end()) {
                queues_for_job[jid].push(tensor_key);
            } else {
                deferred_tensors[jid].push_back(tensor_key);
            }

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
            active_tensor_for_jid.erase(jid);
            auto &tensor_key = req->getTensor_key();
            if (req->getChunk_id() + 1 == req->getNum_chunks()) { // last chunk
                remaining_sizes[tensor_key] = 0;
            } else {
                remaining_sizes[tensor_key] -= chunk_size;
            }
            if (remaining_sizes[tensor_key] == 0) {
                clean_resources_for_tensor(tensor_key);
            }
            num_workers_of_active_tensor_key.erase(tensor_key);
            auto &deque = deferred_tensors[jid];
            auto &pq = queues_for_job[jid];
            while (!deque.empty()) {
                pq.push(deque.front());
                deque.pop_front();
            }
            updatePendingTensors();
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
