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
    ~Switch();
protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
private:
    std::unordered_map<int, int> gate_id { }; // switch or worker id -> gate id
    std::unordered_map<uint64_t, std::unordered_map<std::string, unsigned>> count_for_tensor_key { }; // jid, hash
    std::unordered_map<uint64_t,
            std::unordered_map<std::string, std::unordered_set<unsigned>>> seen_for_tensor_key { };
    std::unordered_map<unsigned, unsigned> num_updates_for_job { };
    std::unordered_map<unsigned, bool> top_level_for_job { };
    std::unordered_map<unsigned, std::unordered_set<int>> gate_ids_for_job { };
    void multicast_downward(SwitchMLPacket*);
    std::unordered_map<int, cMessage *> endTransmissionEvents { };
    std::unordered_map<int, cChannel *> channels { };
    std::unordered_map<int, bool> port_isBusy { };
    std::unordered_map<int, cQueue> queues { };
    void try_send(cPacket *, int);
    void startTransmitting(cMessage *, int);
};

#endif /* SWITCH_H_ */
