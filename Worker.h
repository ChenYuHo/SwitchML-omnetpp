#ifndef WORKER_H_
#define WORKER_H_
#include <omnetpp.h>
#include <unordered_map>
#include <unordered_set>
#include "Switch.h"
#include "SwitchML_m.h"
#include "TrainingProcess.h"
using namespace omnetpp;

class Worker: public cSimpleModule {
public:
    ~Worker();
private:
    friend TrainingProcess;
    cModuleType *srvProcType;
    unsigned free_gpus { 0 };
    std::unordered_map<uint32_t, std::unordered_set<uint32_t>> received_pkts { };
    cGate *out_gate;
    void sendNextPacket(SwitchMLPacket*, uint32_t);
    uint64_t num_slots;
    uint64_t num_updates;
    std::unordered_map<uint64_t, TrainingProcess*> training_process_for_job { };
    cModule *collective_scheduler;
    cModule *job_dispatcher;
    unsigned num_jobs_given { 0 };
    std::unordered_map<uint64_t, cQueue> collective_operation_requests_for_job { };
    std::unordered_map<uint64_t, bool> doing_collective_operation { };
    void startOneCollectiveOperation(uint64_t);
    int64_t MTU;
protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
};

#endif /* WORKER_H_ */
