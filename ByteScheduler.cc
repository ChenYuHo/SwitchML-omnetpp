#include "SwitchML_m.h"
#include "JobDispatcher.h"
#include "ModelStats.h"
#include <unordered_map>
#include <queue>
#define FMT_HEADER_ONLY
#include "fmt/format.h"
using namespace omnetpp;

class ByteScheduler: public cSimpleModule {
private:
    uint64_t chunk_size;
    std::unordered_map<TensorKey, uint64_t> remaining_sizes { };
    std::unordered_map<TensorKey, std::vector<CollectiveOperationRequest*>> requests_of_key { };
    typedef std::pair<uint64_t, uint64_t> layer_tkey_pair;
    std::unordered_map<uint64_t, std::priority_queue<TensorKey>> queues_for_job { };
    std::unordered_map<uint64_t, bool> busy { };
    JobDispatcher *job_dispatcher { };
    std::unordered_map<uint64_t, unsigned> num_workers_of_active_job_id { };
    void StartOneCollectiveOperation(uint64_t);
    void clean_resources_for_job(uint64_t);
    void clean_resources_for_tensor(const TensorKey&);
    void initialize() override;
    void handleMessage(cMessage *msg) override;
};

Define_Module(ByteScheduler);

void ByteScheduler::initialize() {
    chunk_size = par("chunk_size");
    job_dispatcher = (JobDispatcher*) getModuleByPath("^.job_dispatcher");
}

void ByteScheduler::clean_resources_for_tensor(const TensorKey &tensor_key) {
    requests_of_key.erase(tensor_key);
    remaining_sizes.erase(tensor_key);
}

void ByteScheduler::clean_resources_for_job(uint64_t jid) {
    queues_for_job.erase(jid);
    busy.erase(jid);
    num_workers_of_active_job_id.erase(jid);
}

void ByteScheduler::StartOneCollectiveOperation(uint64_t jid) {
    if (busy[jid]) {
        EV_DEBUG << "Job " << jid << " is busy\n";
        return;
    }
    auto &queue = queues_for_job[jid];
    if (queue.empty()) {
        EV_DEBUG << "Job " << jid << " empty queue\n";
        return;
    }
    busy[jid] = true;
    auto &tensor_key = queue.top();
    auto &requests = requests_of_key[tensor_key];
    auto next_chunk_id = requests[0]->getChunk_id() + 1;
    auto n_chunks = requests[0]->getNum_chunks();
    bool last_chunk = next_chunk_id == n_chunks;
    if (last_chunk) {
        for (auto &req : requests) {
            req->setSize(remaining_sizes[tensor_key]);
        }
    }
    EV_DEBUG << "ByteScheduler notifies Workers ";
    for (auto &req : requests) {
        EV_DEBUG << req->getWorker_id() << " ";
        sendDirect(req->dup(), getSimulation()->getModule(req->getWorker_id()),
                "directin");
        req->setChunk_id(next_chunk_id);
    }
    EV_DEBUG << "to start Collective Operation for Job " << tensor_key.job_id
                    << " layer " << tensor_key.layer << ", chunk "
                    << next_chunk_id << "/" << n_chunks << " size "
                    << requests[0]->getSize() << " at " << simTime() << endl;
    num_workers_of_active_job_id[jid] = requests.size();
    if (last_chunk) {
        remaining_sizes[tensor_key] = 0;
        for (auto &req : requests) {
            delete req;
        }
        queue.pop();
    } else {
        remaining_sizes[tensor_key] -= chunk_size;
    }
}

void ByteScheduler::handleMessage(cMessage *msg) {
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
            auto next_size = num_chunks == 1 ? size : chunk_size;
            for (auto &req : requests) {
                req->setSize(next_size);
                req->setNum_chunks(num_chunks);
            }
            // layers nearer the front gets higher priority
            auto jid = request->getTensor_key().job_id;
            queues_for_job[jid].push(tensor_key);
            StartOneCollectiveOperation(jid);
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
            EV_DEBUG << "ByteScheduler Job " << jid << " layer "
                            << tensor_key.layer << " done\n";
            busy[jid] = false;
            if (req->getCompleted()) {
                clean_resources_for_tensor(req->getTensor_key());
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
