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

/**
 * Dynamically launched process in the server; see NED file for more info
 */
class TrainingProcess: public cSimpleModule {
public:
    ~TrainingProcess();
private:
    std::vector<bool> can_do_fp { };
    void allreduce(uint64_t, uint64_t);
    std::hash<std::string> hasher { };
    void waitAndProcessAck(simtime_t, cQueue*);
    void process_ack(LayerAck*);
    cModule *collective_scheduler;
    cModule *job_dispatcher;
    cModule *worker;
    Job *job;
    uint64_t rank;
    uint64_t jid;
    int wid;
    uint64_t iters;
    uint64_t iter = 0;
//    short model;
    bool distributed;
    void finish() override;
    simsignal_t idleTimeSignal;
    simsignal_t iterTimeSignal;
    simsignal_t commTimeSignal;
    std::queue<simtime_t> iter_start;
    std::queue<simtime_t> idle_start;
    std::queue<simtime_t> comm_start;
    unsigned count = 0;
    size_t num_layers;

    void initialize() override;
    void handleMessage(cMessage *msg) override;
    void startIteration(uint64_t);

    std::vector<int64_t> model { };
    std::vector<simtime_t> fp_times { };
    std::vector<simtime_t> bp_times { };
    std::vector<simtime_t> wu_times { };
    std::vector<bool> layer_done { };
};

#endif /* TrainingProcess_H_ */
