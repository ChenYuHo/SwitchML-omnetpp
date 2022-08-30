/*
 * Worker.h
 *
 *  Created on: Aug 29, 2022
 *      Author: root
 */

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
    unsigned get_free_gpus() {
        return free_gpus;
    }
    int get_tor_id();
    void adjust_free_gpus(unsigned offset, bool increment) {
        free_gpus = increment ? free_gpus + offset : free_gpus - offset;
    }
private:
    cModuleType *srvProcType;
    unsigned free_gpus { 0 };
    Switch *tor { nullptr };
    unsigned id;
    void start_job(Job*);
    std::unordered_map<uint32_t, std::unordered_set<uint32_t>> received_pkts { };
    cGate *out_gate;
    void sendNextPacket(SwitchMLPacket*, uint32_t);
    uint64_t num_slots;
    uint64_t num_updates;
    std::unordered_map<uint64_t, TrainingProcess*> training_process_for_job { };

protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
};

#endif /* WORKER_H_ */
