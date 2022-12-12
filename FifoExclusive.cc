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
    std::queue<TensorKey> queue { };
    JobDispatcher *job_dispatcher { };
    std::unordered_map<TensorKey, unsigned> num_workers_of_active_tensor_key { };
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
    auto &tensor_key = queue.front(); // FIFO
    auto &requests = requests_of_key[tensor_key];
    auto jid_to_add = tensor_key.job_id;
    auto layer = tensor_key.layer;
    if (job_dispatcher->accommodate(num_workers_of_active_tensor_key,
            jid_to_add)) {
        // can accommodate, start collective operation
        for (auto req : requests) {
            EV_DEBUG
                            << fmt::format(
                                    "FifoExclusive notifies Worker {} to start Collective Operation for Job {} layer {}/{}, chunk {}/{}\n",
                                    req->getWorker_id(), jid_to_add, layer,
                                    n_layers[req->getModel()],
                                    req->getChunk_id(), req->getNum_chunks());
            sendDirect(req, getSimulation()->getModule(req->getWorker_id()),
                    "directin");
        }
        num_workers_of_active_tensor_key[tensor_key] = requests.size();
        requests_of_key.erase(tensor_key); // sent out, no need to delete pointers
        queue.pop();
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
        auto &tensor_key = req->getTensor_key();
        auto jid = tensor_key.job_id;
        auto layer = tensor_key.layer;

        num_workers_of_active_tensor_key[tensor_key] -= 1;
        EV_DEBUG
                        << fmt::format(
                                "FifoExclusive receives CollectiveOperationRequest layer {} jid {}, remaining workers {}\n",
                                layer, jid,
                                num_workers_of_active_tensor_key[tensor_key]);
        if (num_workers_of_active_tensor_key[tensor_key] == 0) {
            num_workers_of_active_tensor_key.erase(tensor_key);
            while (TryStartOneCollectiveOperation()) {
                // start as many as possible
            }
        }
        delete msg;
        break;
    }
    case 5: { // finished job
        auto jid = ((Job*) msg)->getJob_id();
        for (auto iterator = num_workers_of_active_tensor_key.begin();
                iterator != num_workers_of_active_tensor_key.end();) {
            if (iterator->first.job_id == jid) {
                iterator = num_workers_of_active_tensor_key.erase(iterator);
            } else {
                ++iterator;
            }
        }
        delete msg;
        break;
    }
    default:
        delete msg;
        EV_FATAL << "got unexpected message" << endl;
        break;
    }

}

FifoExclusive::~FifoExclusive() = default;
