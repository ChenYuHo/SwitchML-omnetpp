#include "SwitchML_m.h"
#define STACKSIZE    16384
using namespace omnetpp;

/**
 * Dynamically launched process in the server; see NED file for more info
 */
class TrainingProcess: public cSimpleModule {
public:
    TrainingProcess() :
            cSimpleModule(STACKSIZE) {
    }
    virtual void activity() override;
private:
    std::vector<bool> can_do_fp { };
    void allreduce(JobInfo*);
};

Define_Module(TrainingProcess);

void TrainingProcess::allreduce(JobInfo *msg) {
    EV << "Start Allreduce" << endl;
}

void TrainingProcess::activity() {
    // retrieve parameters
    auto srvProcType = cModuleType::find("Allreducer");
    auto allreducer = srvProcType->createScheduleInit("allreducer",
            getParentModule());
    auto msg = check_and_cast<JobInfo*>(receive());
    auto rank = msg->getRank();
    auto id = msg->getId();
    auto iters = msg->getIters();
    auto num_layers = msg->getGrad_sizesArraySize();
    bool distributed = msg->getNum_workers_allocated() > 1;
    can_do_fp.resize(num_layers, true);
    EV << "Start Job " << id << " as rank " << rank << endl;

    for (unsigned iter = 0; iter < iters; ++iter) {
        for (size_t i = 0; i < num_layers; ++i) {
            while (!can_do_fp[i]) {
                auto ack = check_and_cast<LayerAck*>(receive());
                can_do_fp[ack->getLayer()] = true;
            }
            wait(msg->getFp_times(i));
            can_do_fp[i] = false;
        }

        for (size_t i = num_layers; i > 0; --i) {
            wait(msg->getBp_times(i - 1));
            if (distributed) {
                allreduce(msg);
            } else {

            }
        }
        // wait for num_layers msgs from server

    }

    EV << "exiting\n";
    allreducer->deleteModule();
    deleteModule();
}

