/*
 * JobDispatcher.h
 *
 *  Created on: Aug 29, 2022
 *      Author: root
 */

#ifndef JOBDISPATCHER_H_
#define JOBDISPATCHER_H_

#include <omnetpp.h>
#include <map>
#include <unordered_map>
#include "Worker.h"
#include "Switch.h"
using namespace omnetpp;
class Hierarchy;
class JobScheduling;
class JobPlacement;
class Random;
class TwoLayers;

class JobDispatcher: public cSimpleModule {
public:
    ~JobDispatcher();
private:
    friend Random;
    friend TwoLayers;
    Hierarchy *hierarchy;
    JobScheduling *job_scheduling;
    JobPlacement *job_placement;
    unsigned n_workers;
    unsigned switch_ports;
    std::map<uint64_t, Job*> jobs { }; // jid->job
//    std::vector<Worker*> workers { };
    std::unordered_map<int, Worker*> workers{};
    std::unordered_map<int, Switch*> tors{};
    std::unordered_map<int, int> tor_id_for_worker { };
    std::unordered_map<int, unsigned> free_gpus { }; // worker id -> free gpus

//    typedef Job* (JobDispatcher::*pick_func)(void);
//    pick_func pick_a_job_to_execute;
//    Job* fifo();

//    typedef std::unordered_map<Worker*, unsigned> (JobDispatcher::*place_func)(
//            Job*);
//    place_func get_placement;

//    bool force_distributed { false };
//    bool force_multi_racks { false };

//    std::unordered_map<Worker*, unsigned> random(Job*);

//    typedef void (JobDispatcher::*setup_func)(uint64_t,
//            const std::unordered_map<Worker*, unsigned>&);
//    setup_func setup_switches;
//    void two_layers(uint64_t, const std::unordered_map<Worker*, unsigned>&);
    simsignal_t jctSignal;
    simsignal_t jsmtSignal;
    simsignal_t jstSignal;
    simsignal_t jwtSignal;
    bool tryDispatchAJob();

protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
};

#endif /* JOBDISPATCHER_H_ */
