#ifndef JOBDISPATCHER_H_
#define JOBDISPATCHER_H_

#include <omnetpp.h>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <deque>
#include "Worker.h"
#include "Switch.h"
using namespace omnetpp;
class Hierarchy;
class JobScheduling;
class JobPlacement;
class Random;
class Custom;
class TwoLayers;

class JobDispatcher: public cSimpleModule {
public:
    ~JobDispatcher();
//    bool accommodate(const std::unordered_set<uint64_t>&, uint64_t);
    bool accommodate(const std::unordered_map<TensorKey, unsigned>&, uint64_t,
            bool = true);
    void clean_resources_for_tensor_key(uint64_t, const TensorKey&);
    void bssi(std::deque<TensorKey>&, std::unordered_map<TensorKey, double>,
            const std::unordered_map<TensorKey, uint64_t>&);
private:
    friend Random;
    friend Custom;
    friend TwoLayers;
    Hierarchy *hierarchy;
    JobScheduling *job_scheduling;
    JobPlacement *job_placement;
    cModule *collective_scheduler;
    int n_workers;
    unsigned n_query_results_received = 0;
    unsigned switch_ports;
    std::map<uint64_t, Job*> jobs { }; // jid->job
    std::unordered_map<uint64_t, std::unordered_set<int>> workers_for_job { };
    std::unordered_map<uint64_t, std::unordered_set<int>> switches_for_job { };
    std::unordered_map<int, Worker*> workers { };
    std::unordered_map<int, std::vector<int>> workers_for_tor { };
    std::unordered_map<int, int> index_to_wid { };
//    std::unordered_map<int, Switch*> tors { };
    std::unordered_map<int, int> tor_id_for_worker { };
    std::unordered_map<int, unsigned> free_gpus { }; // worker id -> free gpus
    simsignal_t jobCompletionTime;
    simsignal_t jobSubmissionTime;
    simsignal_t jobStartTime;
    simsignal_t jobWaitTime;
    simsignal_t jobPlacementType;
    bool tryDispatchAJob();

    void initialize(int) override;
    void handleMessage(cMessage *msg) override;
    int numInitStages() const override {
        return 2;
    }

    cIListener *listener;
};

#endif /* JOBDISPATCHER_H_ */
