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
    std::vector<unsigned> can_do_fp { };
    void startComm(uint64_t, uint64_t);
    std::hash<std::string> hasher { };
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
//    short model;
    bool distributed;
    void finish() override;
    simsignal_t idleTimeSignal;
    simsignal_t iterTime;
    simsignal_t commTimeSignal;
    simsignal_t delayTimeSignal;
    simsignal_t realCommTimeSignal;
    std::queue<simtime_t> iter_start;
    std::queue<simtime_t> idle_start;
    std::queue<simtime_t> idle_start_wu;
    std::queue<simtime_t> comm_start;
    std::queue<simtime_t> delay_start;
    simtime_t all_fps_and_last_bp_time;
    simtime_t total_fp_bp_times;
    simtime_t job_start_time;
    unsigned count = 0;
    size_t num_layers;

    void initialize() override;
    void handleMessage(cMessage *msg) override;
    void startIteration(uint64_t);

    std::vector<int64_t> model { };
    std::vector<simtime_t> fp_times { };
    std::vector<simtime_t> bp_times { };
    std::vector<simtime_t> wu_times { };
    std::vector<simtime_t> min_wait_times { };
    std::vector<simtime_t> idle_times { };
    simtime_t total_idle_time = 0; // fp, bp times count as GPU busy
    simtime_t total_idle_time_wu = 0; // fp, bp, wu times count as GPU busy
    unsigned busy_wu = 0;
    bool busy = false;

    std::vector<simtime_t> real_comm_times { };
    std::vector<bool> layer_done { };

    void idleStarts();
    void idleEnds();
    void idleStarts_wu();
    void idleEnds_wu();
    void start_idle();
    void record_idle();
    void start_idle_wu();
    void record_idle_wu();

    simsignal_t fullIterTime;
    simsignal_t contiguousIterTime;
    simsignal_t idleTime;
    simsignal_t idleTimeWu;
    simsignal_t commTime;
    simsignal_t realCommTime;

//    @signal[fullIterTime](type=simtime_t); // per iter, from first FP start to last WU end
//    @signal[contiguousIterTime](type=simtime_t);// per iter, from first FP of this iter start to first FP of the next iter start
//    @signal[idleTime](type=simtime_t);// per iter, time when no FP, BP is ongoing
//    @signal[idleTimeWu](type=simtime_t);// per iter, time when no FP, BP, WU is ongoing
//    @signal[commTime](type=simtime_t);// per iter, from first collective start to last collective end of an iteration
//    @signal[realCommTime](type=simtime_t);
    // per iter, time spend for communication

};

#endif /* TrainingProcess_H_ */
