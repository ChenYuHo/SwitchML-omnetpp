#include "SwitchML_m.h"
#include <unordered_map>
using namespace omnetpp;

class ByteScheduler: public cSimpleModule {
private:
    std::unordered_map<uint64_t, std::vector<CollectiveOperationRequest*>> queue { };
protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
};

Define_Module(ByteScheduler);

void ByteScheduler::initialize() {
}

void ByteScheduler::handleMessage(cMessage *msg) {
    switch (msg->getKind()) {
    case 0: {
        // AllreduceRequest from TrainingProcess
        auto request = check_and_cast<CollectiveOperationRequest*>(msg);
        auto &requests = queue[request->getTensor_key()];
        requests.push_back(request);
        if (requests.size() == request->getNum_workers_allocated()) {
            for (auto req : requests) {
                auto reducer = this->getSimulation()->getModule(
                        req->getWorker_id());
                this->sendDirect(req, reducer, "directin");
            }
            queue.erase(request->getTensor_key());
        }
        break;
    }
    case 2: {
        // LayerAck from Worker, meaning allreduce is done
        delete msg;
        break;
    }
    default:
        delete msg;
        EV_FATAL << "got unexpected message" << endl;
        break;
    }

}

