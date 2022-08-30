#include "SwitchML_m.h"
using namespace omnetpp;

class FifoJobScheduler: public cSimpleModule {
private:
protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
};

Define_Module(FifoJobScheduler);

void FifoJobScheduler::initialize() {
}

void FifoJobScheduler::handleMessage(cMessage *msg) {
    // come and start
    auto job_info = check_and_cast<Job*>(msg);
    EV << simTime() << " job_info " << job_info->getSubmit_time() << endl;




//    if (msg->isSelfMessage()) {
//        // start read csv
//        cModule *mod = srvProcType->createScheduleInit("test", this);
//        EV << "Start Server Process" << endl;
//        sendDirect(msg, mod, "in");
//    } else if (!msg->isPacket()) {
//        // Job
//        EV << "Got Job Info" << endl;
//        cModule *mod = srvProcType->createScheduleInit("test", this);
//        EV << "Start Server Process" << endl;
//        sendDirect(msg, mod, "in");
//    } else {
//        // SwitchMLPacket
//        auto pkt = check_and_cast<AllreduceRequest*>(msg);
//    }
}

