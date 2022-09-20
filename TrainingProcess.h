#ifndef TRAININGPROCESS_H_
#define TRAININGPROCESS_H_

#include "SwitchML_m.h"
#define STACKSIZE    32768
#define FMT_HEADER_ONLY
#include "fmt/format.h"
#include <queue>
using namespace omnetpp;
using namespace std;
class Job;

/**
 * Dynamically launched process in the server; see NED file for more info
 */
class TrainingProcess: public cSimpleModule {
public:
    ~TrainingProcess();
    TrainingProcess() :
            cSimpleModule(STACKSIZE) {
    }
    void activity() override;
private:
    std::vector<bool> can_do_fp { };
    void allreduce(Job*, uint64_t, uint64_t, uint64_t);
    std::hash<std::string> hasher { };
    void waitAndProcessAck(simtime_t, cQueue*);
    void process_ack(LayerAck*);
    cModule *collective_scheduler;
    cModule *worker;
    Job *job;
    void finish() override;
    simsignal_t idleTimeSignal;
    simsignal_t iterTimeSignal;
    simsignal_t commTimeSignal;
    std::queue<simtime_t> iter_start;
    unsigned count = 0;
    size_t num_layers;
};

#endif /* TRAININGPROCESS_H_ */
