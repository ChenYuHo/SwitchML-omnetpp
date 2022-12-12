#ifndef WORKER_H_
#define WORKER_H_
#include <omnetpp.h>
#include <unordered_map>
#include <unordered_set>
#include "SwitchML_m.h"
using namespace omnetpp;

class Worker: public cSimpleModule {
public:
    Worker() : queue("queue", [](cObject *a, cObject *b){
        auto sa = (SwitchMLPacket*) a;
        auto sb = (SwitchMLPacket*) b;
        return sa->getPriority() - sb->getPriority();
    }) {

    }
    ~Worker() override;
    int tor_id() {
        return ToR->getId();
    }
    ;
private:
    bool packet_simulation;
    cModule *ToR;
    cModuleType *srvProcType;
    std::unordered_map<TensorKey, std::unordered_set<uint32_t>> received_pkts { };
    cGate *out_gate;
    void sendNextPacket(SwitchMLPacket*, uint32_t);
    uint64_t num_slots;
    uint64_t num_updates;
    std::unordered_map<uint64_t, cModule*> training_process_for_job { };
    cModule *collective_scheduler;
    cModule *job_dispatcher;
    unsigned num_jobs_given { 0 };
    std::unordered_map<uint64_t, cQueue> collective_operation_requests_for_job { };
    std::unordered_map<uint64_t, cMessage*> active_collective_operation_request_for_job { };
    std::unordered_map<uint64_t, bool> doing_collective_operation { };
    void startOneCollectiveOperation(uint64_t);
    int64_t MTU;
    cMessage *endTransmissionEvent;
    bool retransmission_enabled;
    std::unordered_map<TensorKey, int> tensor_priority { };
    std::unordered_map<TensorKey, simtime_t> obsolete_pkt_timestamp { };
    std::vector<SwitchMLPacket*> retransmission_pkts;
    simtime_t retransmission_timeout;
    void schedule_timeout_retransmission(SwitchMLPacket*);
    bool isBusy;
    void startTransmitting(SwitchMLPacket*);
    void try_send(SwitchMLPacket*);
    cPacketQueue queue;
    cChannel *channel;
    void initialize() override;
    void handleMessage(cMessage *msg) override;
    void notifyCollectiveOperationDone(CollectiveOperationRequest*);
    int64_t gbps;

    simsignal_t pktOut;
    simsignal_t pktRetransmission;
//    simsignal_t pktIn;
};

#endif /* WORKER_H_ */
