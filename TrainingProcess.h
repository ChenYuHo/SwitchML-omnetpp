#ifndef TRAININGPROCESS_H_
#define TRAININGPROCESS_H_

#include "SwitchML_m.h"
#define STACKSIZE    16384
#define FMT_HEADER_ONLY
#include "fmt/format.h"
#include "Allreducer.h"
using namespace omnetpp;
using namespace std;
class Worker;

/**
 * Dynamically launched process in the server; see NED file for more info
 */
class TrainingProcess: public cSimpleModule {
public:
    TrainingProcess() :
            cSimpleModule(STACKSIZE) {
    }
    virtual void activity() override;
    void forward_ack(LayerAck*);
    simtime_t get_weight_update_time(size_t layer) {
        return weight_update_time[layer];
    }
private:
    Allreducer *allreducer;
    std::vector<bool> can_do_fp { };
    void allreduce(Job*, uint64_t, uint64_t, uint64_t);
    void setup(Job*);
    std::vector<simtime_t> forward_pass_time;
    std::vector<simtime_t> backward_pass_time;
    std::vector<simtime_t> weight_update_time;
    std::vector<uint64_t> model;
    std::hash<std::string> hasher { };
    uint64_t n_workers;
    void waitAndProcessAck(simtime_t, cQueue*);
    void process_ack(LayerAck*);
    cModule* collective_scheduler;
    Worker* worker;
};

#endif /* TRAININGPROCESS_H_ */
