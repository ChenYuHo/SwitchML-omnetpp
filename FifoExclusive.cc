#include "SwitchML_m.h"
#include "JobDispatcher.h"
#include "ModelStats.h"
#include <unordered_map>
#include <queue>
#define FMT_HEADER_ONLY
#include "fmt/format.h"
using namespace omnetpp;

class FifoExclusive: public cSimpleModule {
public:
    ~FifoExclusive() override;
private:
    std::unordered_map<TensorKey, std::vector<CollectiveOperationRequest*>> requests_of_key { };
    std::queue<uint64_t> queue { };
    JobDispatcher *job_dispatcher { };
    std::unordered_map<uint64_t, unsigned> num_workers_of_active_job_id { };
    bool TryStartOneCollectiveOperation();
    void initialize() override;
    void handleMessage(cMessage *msg) override;
};

Define_Module(FifoExclusive);

void FifoExclusive::initialize() {
    job_dispatcher = (JobDispatcher*) getModuleByPath("^.job_dispatcher");
}

bool FifoExclusive::TryStartOneCollectiveOperation() {
    if (queue.empty())
        return false;
    auto tensor_key = queue.front(); // FIFO
    auto &requests = requests_of_key[tensor_key];
    auto jid_to_add = requests[0]->getJob_id();
    if (job_dispatcher->accommodate(num_workers_of_active_job_id, jid_to_add)) {
        // can accommodate, start collective operation
        for (auto req : requests) {
            EV_DEBUG
                            << fmt::format(
                                    "FifoExclusive notifies Worker {} to start Collective Operation for Job {} layer {}/{}, chunk {}/{}\n",
                                    req->getWorker_id(), req->getJob_id(),
                                    req->getLayer(), n_layers(req->getModel()),
                                    req->getChunk_id(), req->getNum_chunks());
            sendDirect(req, getSimulation()->getModule(req->getWorker_id()),
                    "directin");
        }
        num_workers_of_active_job_id[jid_to_add] = requests.size();
        queue.pop();
        requests_of_key.erase(tensor_key); // sent out, no need to delete pointers
        return true;
    } else
        return false;
}

void FifoExclusive::handleMessage(cMessage *msg) {
    switch (msg->getKind()) {
    case 0: {
        // CollectiveOperationRequest from TrainingProcess
        auto request = (CollectiveOperationRequest*) (msg);
        auto &requests = requests_of_key[request->getTensor_key()];
        requests.push_back(request);
        if (requests.size() == request->getNum_workers_allocated()) {
            queue.push(request->getTensor_key());
            while (TryStartOneCollectiveOperation()) {
                // start as many as possible
            }
        }
        break;
    }
    case 2: {
        // CollectiveOperationRequest from Worker, meaning a collective operation is done
        auto req = (CollectiveOperationRequest*) msg;
        auto jid = req->getJob_id();

        num_workers_of_active_job_id[jid] -= 1;
        EV_DEBUG
                        << fmt::format(
                                "FifoExclusive receives CollectiveOperationRequest layer {} jid {}, remaining workers {}\n",
                                req->getLayer(), jid,
                                num_workers_of_active_job_id[jid]);
        if (num_workers_of_active_job_id[jid] == 0) {
            num_workers_of_active_job_id.erase(jid);
            while (TryStartOneCollectiveOperation()) {
                // start as many as possible
            }
        }
        delete msg;
        break;
    }
    case 5: // finished job
        num_workers_of_active_job_id.erase(((Job*) msg)->getJob_id());
        delete msg;
        break;
    default:
        delete msg;
        EV_FATAL << "got unexpected message" << endl;
        break;
    }

}

FifoExclusive::~FifoExclusive() = default;
