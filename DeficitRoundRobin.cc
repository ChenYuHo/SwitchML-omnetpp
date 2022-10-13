#include "SwitchML_m.h"
#include "JobDispatcher.h"
#include "ModelStats.h"
#include <unordered_map>
#include <queue>
#include <utility>
#define FMT_HEADER_ONLY
#include "fmt/format.h"
using namespace omnetpp;

// DeficitRoundRobin repeatedly services every queue (priority queue of a training job) with one chunk
// and applies work conservation whenever possible
class DeficitRoundRobin: public cSimpleModule {
private:
//    int64_t quantum = 0; // records starting quantum for new entries
    uint64_t chunk_size;
    std::unordered_map<uint64_t, std::priority_queue<TensorKey>> queues_for_job { };
    std::unordered_map<TensorKey, uint64_t> remaining_sizes { };
    std::unordered_map<TensorKey, std::vector<CollectiveOperationRequest*>> requests_of_key { };
    std::unordered_set<uint64_t> jid_set { };
    std::vector<std::pair<int64_t, uint64_t>> drr_queue { }; // set of pair (served chunks (quantum) -> job id), ordered by quantum

    std::deque<uint64_t> jid_queue { };

//    std::unordered_map<uint64_t, bool> busy { };
    JobDispatcher *job_dispatcher { };
    void clean_resources_for_job(uint64_t);
    void clean_resources_for_tensor(const TensorKey&);
    void initialize() override;
    void handleMessage(cMessage *msg) override;
//    void order_and_run();
//    void updatePendingTensors();
    double get_weight(const TensorKey&);
    unsigned StartCollectiveOperations();
    std::deque<TensorKey> pending_tensors { };
    std::unordered_map<uint64_t, unsigned> num_workers_of_active_job_id { }; // only one will be active
};

Define_Module(DeficitRoundRobin);

void DeficitRoundRobin::initialize() {
    chunk_size = par("chunk_size");
    job_dispatcher = (JobDispatcher*) getModuleByPath("^.job_dispatcher");
}

void DeficitRoundRobin::clean_resources_for_tensor(
        const TensorKey &tensor_key) {
    for (auto &req : requests_of_key[tensor_key]) {
        delete req;
    }
    requests_of_key.erase(tensor_key);
    remaining_sizes.erase(tensor_key);
}

void DeficitRoundRobin::clean_resources_for_job(uint64_t jid) {
    queues_for_job.erase(jid);
    num_workers_of_active_job_id.erase(jid);
    jid_set.erase(jid);
    for (auto iter = drr_queue.begin(); iter != drr_queue.end();) {
        if (iter->second == jid) {
            iter = drr_queue.erase(iter);
            break;
        } else {
            iter++;
        }
    }
}

double DeficitRoundRobin::get_weight(const TensorKey &tensor_key) {
//    auto req = requests_of_key[tensor_key];
    // remaining size
    return double(remaining_sizes[tensor_key]);
}

unsigned DeficitRoundRobin::StartCollectiveOperations() {
    if (drr_queue.empty())
        return 0;
    unsigned started = 0;
    // remove completed tensors
    EV_DEBUG << "queue: ";
    for (auto iter = drr_queue.begin(); iter != drr_queue.end();) {
        auto &pq = queues_for_job[iter->second];
        while (!pq.empty() && remaining_sizes[pq.top()] == 0) {
            pq.pop();
        }
        if (pq.empty()) {
            jid_set.erase(iter->second);
            iter = drr_queue.erase(iter);
        } else {
            EV_DEBUG << iter->first << " q j " << iter->second << " ";
            iter++;
        }
    }
    EV_DEBUG << '\b' << endl;

    // will service drr_queue[0] and others if can satisfy work conservation
    auto last_size = (uint64_t) -1; // 2^64-1, largest uint64_t
    for (auto iter = drr_queue.rbegin(); iter != drr_queue.rend(); ++iter) {
        auto jid_to_add = iter->second;
        auto &pq = queues_for_job[jid_to_add];
        if (!pq.empty()) {
            auto &tensor_key = pq.top();
            auto &requests = requests_of_key[tensor_key];
            if (job_dispatcher->accommodate(num_workers_of_active_job_id,
                    jid_to_add)) {
                auto this_size = std::min(remaining_sizes[tensor_key],
                        chunk_size);
                if (this_size <= last_size) {
                    iter->first -= 1;
                    // add to active
                    last_size = this_size; // to ensure strict ordering: work conservation doesn't take larger tensors
                    started++;
                    auto next_chunk_id = requests[0]->getChunk_id() + 1;
                    bool last_chunk = next_chunk_id
                            == requests[0]->getNum_chunks();
                    for (auto &req : requests) {
                        if (last_chunk) {
                            req->setSize(remaining_sizes[tensor_key]);
                        }
                        EV_DEBUG
                                        << fmt::format(
                                                "DeficitRoundRobin notifies Worker {} to start Collective Operation for Job {} layer {}, chunk {}/{} size {}\n",
                                                req->getWorker_id(), jid_to_add,
                                                tensor_key.layer, next_chunk_id,
                                                req->getNum_chunks(),
                                                req->getSize());
                        sendDirect(req->dup(),
                                getSimulation()->getModule(req->getWorker_id()),
                                "directin");
                        req->setChunk_id(next_chunk_id);
                    }
                    num_workers_of_active_job_id[jid_to_add] = requests.size();
                    if (last_chunk) {
                        remaining_sizes[tensor_key] = 0;
                    } else {
                        remaining_sizes[tensor_key] -= chunk_size;
                    }
                }
            }
        } else {
            iter->first = drr_queue.rbegin()->first;
        }
    }
    std::sort(drr_queue.begin(), drr_queue.end());
    return started;
}

void DeficitRoundRobin::handleMessage(cMessage *msg) {
    switch (msg->getKind()) {
    case 0: {
        // CollectiveOperationRequest from TrainingProcess
        auto request = (CollectiveOperationRequest*) (msg);
        auto &tensor_key = request->getTensor_key();
        auto &requests = requests_of_key[tensor_key];

        requests.push_back(request);
        if (requests.size() == request->getNum_workers_allocated()) {
            auto jid = tensor_key.job_id;
            if (jid_set.find(jid) == jid_set.end()) {
                // not found
                auto q = drr_queue.empty() ? 0 : drr_queue.back().first;
                drr_queue.push_back(std::make_pair(q, jid));
                jid_set.insert(jid);
//            } else if (queues_for_job[jid].empty()) {
            }
            auto size = request->getSize();
            remaining_sizes[tensor_key] = size;
            auto num_chunks = size / chunk_size + (size % chunk_size ? 1 : 0);
            for (auto req : requests) {
                req->setSize(chunk_size);
                req->setNum_chunks(num_chunks);
            }
            // layers nearer the front (smaller index) gets higher priority
            queues_for_job[jid].push(tensor_key);
            if (num_workers_of_active_job_id.empty()) {
                StartCollectiveOperations();
            }
        }
        break;
    }
    case 2: {
        // CollectiveOperationRequest from Worker, meaning a collective operation is done
        auto req = (CollectiveOperationRequest*) msg;
        auto &tensor_key = req->getTensor_key();
        auto jid = tensor_key.job_id;
        num_workers_of_active_job_id[jid] -= 1;
        if (num_workers_of_active_job_id[jid] == 0) {
            EV_DEBUG << "Job " << jid << " layer " << tensor_key.layer
                            << " done\n";
            auto &tensor_key = req->getTensor_key();
            if (remaining_sizes[tensor_key] == 0) {
                clean_resources_for_tensor(tensor_key);
            }
            num_workers_of_active_job_id.erase(jid);
            if (num_workers_of_active_job_id.empty()) {
                StartCollectiveOperations();
            }
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
