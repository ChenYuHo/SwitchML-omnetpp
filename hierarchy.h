#ifndef HIERARCHY_H_
#define HIERARCHY_H_

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <omnetpp.h>

class JobDispatcher;
class HierarchyQuery;
class Job;

class Hierarchy {
public:
    virtual ~Hierarchy() = default;
    virtual void setup_job(Job*, const std::unordered_map<int, unsigned>&) = 0;
    virtual std::unordered_set<int> switch_ids_beyond_tors(
            std::unordered_set<int>) {
        return std::unordered_set<int> { };
    }
};

class TwoLayers: public Hierarchy {
public:
    TwoLayers(JobDispatcher *job_dispatcher);
    void setup_job(Job*, const std::unordered_map<int, unsigned>&) override;
    std::unordered_set<int> switch_ids_beyond_tors(std::unordered_set<int>)
            override;
private:
    omnetpp::cModule *core_switch;
    int core_switch_id;
    JobDispatcher *job_dispatcher;
};

#endif /* HIERARCHY_H_ */
