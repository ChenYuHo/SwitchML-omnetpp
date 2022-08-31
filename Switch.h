/*
 * Switch.h
 *
 *  Created on: Aug 29, 2022
 *      Author: root
 */

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
    void set_num_updates_for_job(uint64_t job_id, uint64_t num_updates) {
        num_updates_for_job[job_id] = num_updates;
    }
    void set_top_level_for_job(uint64_t job_id, bool top_level) {
        top_level_for_job[job_id] = top_level;
    }
    void set_gate_ids_for_job(uint64_t,
            const std::unordered_map<Worker*, unsigned>&);
    void set_gate_ids_for_job(uint64_t,
                const std::unordered_map<int, uint64_t>&);
protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
private:
    std::unordered_map<uint64_t, std::unordered_map<std::string, unsigned>> count_for_tensor_key { }; // jid, hash
    std::unordered_map<uint64_t,
            std::unordered_map<std::string, std::unordered_set<unsigned>>> seen_for_tensor_key { };
    std::unordered_map<unsigned, unsigned> num_updates_for_job { };
    std::unordered_map<unsigned, bool> top_level_for_job { };
    std::unordered_map<unsigned, std::unordered_set<int>> gate_ids_for_job { };
    void multicast_downward(SwitchMLPacket*);
};

#endif /* SWITCH_H_ */
