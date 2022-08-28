#include "SwitchML_m.h"
using namespace omnetpp;

class Worker: public cSimpleModule {
private:
    cModuleType *srvProcType;

protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
};

Define_Module(Worker);

void Worker::initialize() {
    srvProcType = cModuleType::find("TrainingProcess");
//    scheduleAt(simTime(), new cMessage);
}

void Worker::handleMessage(cMessage *msg) {
    if (msg->isSelfMessage()) {
        // start SwitchML
        cModule *mod = srvProcType->createScheduleInit("test", this);
        EV << "Start Server Process" << endl;
        sendDirect(msg, mod, "in");
    } else if (!msg->isPacket()) {
        // Job
        EV << "Got Job Info" << endl;
        cModule *mod = srvProcType->createScheduleInit("test", this);
        EV << "Start Server Process" << endl;
        sendDirect(msg, mod, "in");
    } else {
        // SwitchMLPacket
        auto pkt = check_and_cast<AllreduceRequest*>(msg);
    }
}

