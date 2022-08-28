#include "SwitchML_m.h"
using namespace omnetpp;

#define STACKSIZE    16384
class JobDispatcher: public cSimpleModule {
public:
    JobDispatcher() :
            cSimpleModule(STACKSIZE) {
    }
    virtual void activity() override;
};

Define_Module(JobDispatcher);

void JobDispatcher::activity() {
    EV << "Started\n";
    // read csv
    // construct job
    // schedule job
    // place job
    auto *job = new JobInfo;
    job->setGrad_sizesArraySize(2);
    job->setFp_timesArraySize(2);
    job->setBp_timesArraySize(2);
    job->setId(0);
    int n_workers = getParentModule()->par("n_workers");
    for (int i = 0; i < n_workers; ++i) {
        auto j = job->dup();
        j->setRank(i);
        sendDirect(j,
                getParentModule()->getSubmodule("workers", i)->gate("msg$i"));
    }
}
