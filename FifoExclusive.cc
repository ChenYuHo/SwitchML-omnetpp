#include "SwitchML_m.h"
#include "JobDispatcher.h"
#include <unordered_map>
#include <queue>
using namespace omnetpp;

class FifoExclusive: public cSimpleModule {
private:
    std::unordered_map<uint64_t, std::vector<CollectiveOperationRequest*>> requests_of_key { };
    bool busy;
    std::queue<uint64_t> queue { };
    JobDispatcher *job_dispatcher;
    std::unordered_set<uint64_t> active_job_ids { };
    void TryStartOneCollectiveOperation();
protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
};

Define_Module(FifoExclusive);

void FifoExclusive::TryStartOneCollectiveOperation() {
    auto tensor_key = queue.front();
    auto &requests = requests_of_key[tensor_key];
    auto jid_to_add = requests[0]->getJob_id();
    if (job_dispatcher->accommodate(active_job_ids, jid_to_add)) {
        // can accommodate, start collective operation
        for (auto req : requests) {
            sendDirect(req, getSimulation()->getModule(req->getWorker_id()), "directin");
        }
        active_job_ids.insert(jid_to_add);
        queue.pop();
        requests_of_key.erase(tensor_key);
    }
}

void FifoExclusive::initialize() {
    job_dispatcher = (JobDispatcher*) getModuleByPath("^.job_dispatcher");
}

void FifoExclusive::handleMessage(cMessage *msg) {
    switch (msg->getKind()) {
    case 0: {
        // CollectiveOperationRequest from TrainingProcess
        auto request = check_and_cast<CollectiveOperationRequest*>(msg);
        auto &requests = requests_of_key[request->getTensor_key()];
        requests.push_back(request);
        if (requests.size() == request->getNum_workers_allocated()) {
            queue.push(request->getTensor_key());
            TryStartOneCollectiveOperation();
        }
        break;
    }
    case 2: {
        // LayerAck from Worker, meaning a collective operation is done
        active_job_ids.erase(((CollectiveOperationRequest*) msg)->getJob_id());
        TryStartOneCollectiveOperation();
        delete msg;
        break;
    }
    default:
        delete msg;
        EV_FATAL << "got unexpected message" << endl;
        break;
    }

}

