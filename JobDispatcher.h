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
    bool accommodate(unordered_set<uint64_t>, uint64_t);
    bool accommodate(unordered_map<uint64_t, unsigned>, uint64_t);
private:
    friend Random;
    friend TwoLayers;
    Hierarchy *hierarchy;
    JobScheduling *job_scheduling;
    JobPlacement *job_placement;
    unsigned n_workers;
    unsigned n_query_results_received = 0;
    unsigned switch_ports;
    std::map<uint64_t, Job*> jobs { }; // jid->job
    std::unordered_map<uint64_t, std::unordered_set<int>> workers_for_job { };
    std::unordered_map<uint64_t, std::unordered_set<int>> switches_for_job { };
    std::unordered_map<int, Worker*> workers { };
    std::unordered_map<int, int> ports { };
    std::unordered_map<int, Switch*> tors { };
    std::unordered_map<int, int> tor_id_for_worker { };
    std::unordered_map<int, unsigned> free_gpus { }; // worker id -> free gpus
    simsignal_t jctSignal;
    simsignal_t jsmtSignal;
    simsignal_t jstSignal;
    simsignal_t jwtSignal;
    simsignal_t jpSignal;
    bool tryDispatchAJob();

protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
};

#endif /* JOBDISPATCHER_H_ */
