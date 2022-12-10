#ifndef SWITCH_H_
#define SWITCH_H_
#include <omnetpp.h>
#include <unordered_map>
#include <unordered_set>
#include "SwitchML_m.h"
using namespace omnetpp;
class Worker;
class Switch: public cSimpleModule {
public:
    void clean_resources_for_tensor(const TensorKey&);
    void clean_resources_for_job(uint64_t);
    ~Switch() override;
    void initialize() override;
    void handleMessage(cMessage *msg) override;
private:
    std::unordered_map<int, int> gate_id { }; // switch or worker id -> gate id
    std::unordered_map<TensorKey, std::unordered_map<std::string, unsigned>> count_for_tensor_key { }; // jid, hash
    std::unordered_map<TensorKey,
            std::unordered_map<std::string, std::unordered_set<unsigned>>> seen_for_tensor_key { };
    std::unordered_map<unsigned, unsigned> num_updates_for_job { };
    std::unordered_map<unsigned, bool> top_level_for_job { };
    std::unordered_map<unsigned, std::unordered_set<int>> gate_ids_for_job { };
    std::unordered_map<int, cMessage*> endTransmissionEvents { }; // gate id -> message
    std::unordered_map<int, cChannel*> channels { }; // gate id -> channel
    std::unordered_map<int, bool> port_isBusy { };
    std::unordered_map<int, cQueue> queues { };
    std::unordered_set<uint64_t> done_jobs { };
    void multicast_downward(SwitchMLPacket*);
    void try_send(cPacket*, int);
    void startTransmitting(cMessage*, int);
    Switch *upper_level_switch;
};

#endif /* SWITCH_H_ */
