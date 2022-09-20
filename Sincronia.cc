#include "SwitchML_m.h"
#include "JobDispatcher.h"
#include "ModelStats.h"
#include <unordered_map>
#include <queue>
using namespace omnetpp;

class Sincronia: public cSimpleModule {
private:
    uint64_t chunk_size;
    std::unordered_map<uint64_t, uint64_t> remaining_size { };
    std::unordered_map<uint64_t, std::vector<CollectiveOperationRequest*>> requests_of_key { };
    typedef std::pair<uint64_t, uint64_t> layer_tkey_pair;
    std::unordered_map<uint64_t,
            std::priority_queue<layer_tkey_pair, std::vector<layer_tkey_pair>,
                    std::greater<layer_tkey_pair>>> queues_for_job { };
//    std::unordered_map<uint64_t, bool> busy { };
    JobDispatcher *job_dispatcher { };
    std::unordered_map<uint64_t, unsigned> num_workers_of_active_job_id { };
    void StartOneCollectiveOperation(uint64_t);
    void clean_resources_for_job(uint64_t);
    void clean_resources_for_tensor(uint64_t);
    void initialize() override;
    void handleMessage(cMessage *msg) override;
    void order_and_run();
    double get_weight(uint64_t);
    unsigned StartCollectiveOperations();
    std::deque<uint64_t> pending_tensors { };
};

Define_Module(Sincronia);

void Sincronia::initialize() {
    chunk_size = par("chunk_size");
    job_dispatcher = (JobDispatcher*) getModuleByPath("^.job_dispatcher");
}

void Sincronia::clean_resources_for_tensor(uint64_t tensor_key) {
    for (auto req : requests_of_key[tensor_key]) {
        delete req;
    }
    requests_of_key.erase(tensor_key);
    remaining_size.erase(tensor_key);
}

void Sincronia::clean_resources_for_job(uint64_t jid) {
    queues_for_job.erase(jid);
//    busy.erase(jid);
    num_workers_of_active_job_id.erase(jid);
}

void Sincronia::StartOneCollectiveOperation(uint64_t tensor_key) {
//    if (busy[jid]) {
//        EV_DEBUG << "Job " << jid << " is busy\n";
//        return;
//    }
//    auto &queue = queues_for_job[jid];
//    if (queue.empty()) {
//        EV_DEBUG << "Job " << jid << " empty queue\n";
//        return;
//    }
//    busy[jid] = true;
//    auto tensor_key = queue.top().second;
    auto &requests = requests_of_key[tensor_key];
    auto next_chunk_id = requests[0]->getChunk_id() + 1;
    bool last_chunk = next_chunk_id == requests[0]->getNum_chunks();
    if (last_chunk) {
        for (auto req : requests) {
            req->setSize(remaining_size[tensor_key]);
        }
    }
    for (auto req : requests) {
        EV_DEBUG
                        << fmt::format(
                                "Sincronia notifies Worker {} to start Collective Operation for Job {} layer {}, chunk {}/{} size {}\n",
                                req->getWorker_id(), req->getJob_id(),
                                req->getLayer(), next_chunk_id,
                                req->getNum_chunks(), req->getSize());
        sendDirect(req->dup(), getSimulation()->getModule(req->getWorker_id()),
                "directin");
        req->setChunk_id(next_chunk_id);
    }
//    num_workers_of_active_job_id[jid] = requests.size();
//    if (last_chunk) {
//
//        remaining_size[tensor_key] = 0;
//    } else {
//        remaining_size[tensor_key] -= chunk_size;
//    }
}

double Sincronia::get_weight(uint64_t tensor_key) {
//    auto req = requests_of_key[tensor_key];
    // remaining size
    return double(remaining_size[tensor_key]);
}

unsigned Sincronia::StartCollectiveOperations() {
    if (pending_tensors.empty())
        return 0;

    for (auto iterator = pending_tensors.begin();
            iterator != pending_tensors.end();) {
        auto tensor_key = *iterator;
        auto &requests = requests_of_key[tensor_key];
        auto jid_to_add = requests[0]->getJob_id();
        if (job_dispatcher->accommodate(num_workers_of_active_job_id,
                jid_to_add)) {
            num_workers_of_active_job_id[jid_to_add] = requests.size();
            iterator = pending_tensors.erase(iterator);
        } else {
            ++iterator;
        }
    }

    // invoke

}

void Sincronia::order_and_run() {
    if (!pending_tensors.empty())
        return;
    std::unordered_map<uint64_t, double> weights { }; // tensor_key -> weight
    for (const auto &pair : queues_for_job) {
        auto &pq = pair.second;
        if (!pq.empty()) {
//            weights[pq.top()] = get_weight(pq.top());
        }
    }
    if (weights.empty())
        return;
    // bssi
    if (weights.size() > 1) {
        job_dispatcher->bssi(pending_tensors, weights);
    } else {
        pending_tensors.push_back(weights.begin()->first);
    }
    auto started = StartCollectiveOperations();
}

void Sincronia::handleMessage(cMessage *msg) {
    switch (msg->getKind()) {
    case 0: {
        // CollectiveOperationRequest from TrainingProcess
        auto request = (CollectiveOperationRequest*) (msg);
        auto &requests = requests_of_key[request->getTensor_key()];
        requests.push_back(request);
        EV_DEBUG << requests.size() << " "
                        << request->getNum_workers_allocated() << endl;
        if (requests.size() == request->getNum_workers_allocated()) {
            auto tensor_key = request->getTensor_key();
            auto size = request->getSize();
            remaining_size[tensor_key] = size;
            auto num_chunks = size / chunk_size + (size % chunk_size ? 1 : 0);
            for (auto req : requests) {
                req->setSize(chunk_size);
                req->setNum_chunks(num_chunks);
            }
            // layers nearer the front gets higher priority
            auto jid = request->getJob_id();
            queues_for_job[jid].push(
                    std::make_pair(request->getLayer(), tensor_key));
            StartOneCollectiveOperation(jid);
        }
        break;
    }
    case 1:
        // run loop
        break;
    case 2: {
        // LayerAck from Worker, meaning a collective operation is done
        auto ack = (LayerAck*) msg;
        auto jid = ack->getJob_id();
        num_workers_of_active_job_id[jid] -= 1;
        if (num_workers_of_active_job_id[jid] == 0) {
            EV_DEBUG << "Job " << ack->getJob_id() << " layer "
                            << ack->getLayer() << " done\n";
//            busy[jid] = false;
            if (ack->getCompleted()) {
                clean_resources_for_tensor(ack->getTensor_key());
            }
            StartOneCollectiveOperation(jid);
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
