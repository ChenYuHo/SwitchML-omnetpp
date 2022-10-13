#ifndef TrainingProcess_H_
#define TrainingProcess_H_

#include "SwitchML_m.h"
#define FMT_HEADER_ONLY
#include "fmt/format.h"
#include <queue>
using namespace omnetpp;
using namespace std;
class Worker;
class Job;

class TrainingProcess: public cSimpleModule {
public:
    ~TrainingProcess();
private:
    std::vector<unsigned> can_do_fp { };
    void startComm(uint64_t, uint64_t);
    cModule *collective_scheduler;
    cModule *job_dispatcher;
    cModule *worker;
    Job *job;
    uint64_t rank;
    uint64_t jid;
    int wid;
    uint64_t iters;
    uint64_t iter = 0;
    uint64_t datarate;
    uint32_t num_workers_allocated;
    bool distributed;
    void finish() override;
    std::queue<simtime_t> iter_start;
    simtime_t gpu_start_idle_time;

    void initialize() override;
    void handleMessage(cMessage *msg) override;
    void startIteration(uint64_t);

    short model;
    std::vector<simtime_t> real_comm_times { };
    std::vector<bool> layer_done { };

    void markIdleStart();
    void recordIdleTimeIfAny();

    simsignal_t fullIterTime;
    simsignal_t contiguousIterTime;
    simsignal_t minIdleTime;
    simsignal_t minIdleTimeWu;
    simsignal_t idleTime;
    simsignal_t idleTimeWu;
    simsignal_t commTime;
    simsignal_t realCommTime;
    simsignal_t workerJobCompletionTime;
};

#endif /* TrainingProcess_H_ */
