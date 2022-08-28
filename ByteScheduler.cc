#include "CollectiveScheduler.h"
#include "SwitchML_m.h"

using namespace omnetpp;

class ByteScheduler: public CollectiveScheduler {
public:
    void enqueue() override;
private:

protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
};

Define_Module(ByteScheduler);

void ByteScheduler::enqueue() {

}

void ByteScheduler::initialize() {
//    scheduleAt(simTime(), new cMessage);
}

void ByteScheduler::handleMessage(cMessage *msg) {

}
