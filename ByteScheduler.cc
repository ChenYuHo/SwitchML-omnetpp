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
    std::unordered_map<TensorKey, uint64_t> original_size { };
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
    original_size.erase(tensor_key);
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
    bool last_chunk = next_chunk_id == requests[0]->getNum_chunks();
    if (last_chunk) {
        for (auto &req : requests) {
            req->setSize(original_size[tensor_key] % chunk_size);
        }
    }
    for (auto &req : requests) {
        EV_DEBUG
                        << fmt::format(
                                "ByteScheduler notifies Worker {} to start Collective Operation for Job {} layer {}, chunk {}/{} size {}\n",
                                req->getWorker_id(), req->getJob_id(),
                                req->getLayer(), next_chunk_id,
                                req->getNum_chunks(), req->getSize());
        sendDirect(req->dup(), getSimulation()->getModule(req->getWorker_id()),
                "directin");
        req->setChunk_id(next_chunk_id);
    }
    num_workers_of_active_job_id[jid] = requests.size();
    if (last_chunk) {
        for (auto &req : requests) {
            delete req;
        }
        queue.pop();
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
        EV_DEBUG << requests.size() << " "
                        << request->getNum_workers_allocated() << endl;
        if (requests.size() == request->getNum_workers_allocated()) {
            auto size = request->getSize();
            original_size[tensor_key] = size;
            auto num_chunks = size / chunk_size + (size % chunk_size ? 1 : 0);
            for (auto &req : requests) {
                req->setSize(chunk_size);
                req->setNum_chunks(num_chunks);
            }
            // layers nearer the front gets higher priority
            auto jid = request->getJob_id();
            queues_for_job[jid].push(tensor_key);
            StartOneCollectiveOperation(jid);
        }
        break;
    }
    case 2: {
        // CollectiveOperationRequest from Worker, meaning a collective operation is done
        auto req = (CollectiveOperationRequest*) msg;
        auto jid = req->getJob_id();
        num_workers_of_active_job_id[jid] -= 1;
        if (num_workers_of_active_job_id[jid] == 0) {
            EV_DEBUG << "[ByteScheduler] ]Job " << jid << " layer "
                            << req->getLayer() << " done\n";
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
