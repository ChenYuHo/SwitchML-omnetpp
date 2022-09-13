#ifndef TRAININGPROCESS_H_
#define TRAININGPROCESS_H_

#include "SwitchML_m.h"
#define STACKSIZE    16384
#define FMT_HEADER_ONLY
#include "fmt/format.h"
using namespace omnetpp;
using namespace std;
class Worker;
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
    virtual void activity() override;
private:
    std::vector<bool> can_do_fp { };
    void allreduce(Job*, uint64_t, uint64_t, uint64_t);
    std::vector<simtime_t> forward_pass_time;
    std::vector<simtime_t> backward_pass_time;
    std::vector<simtime_t> weight_update_time;
    std::vector<uint64_t> model;
    std::hash<std::string> hasher { };
    void waitAndProcessAck(simtime_t, cQueue*);
    void process_ack(LayerAck*);
    cModule *collective_scheduler;
    Worker *worker;
    Job *job;
};

#endif /* TRAININGPROCESS_H_ */
