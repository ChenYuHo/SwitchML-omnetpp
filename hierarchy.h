#ifndef HIERARCHY_H_
#define HIERARCHY_H_

#include <vector>
#include <unordered_map>
#include <unordered_set>

class Switch;
class JobDispatcher;
class HierarchyQuery;
class Job;

class Hierarchy {
public:
    virtual ~Hierarchy() = default;
    virtual void setup_job(Job*, const std::unordered_map<int, unsigned>&) = 0;
    virtual void process_hierarchy_query(HierarchyQuery *q) {
    }
    virtual std::unordered_set<int> switch_ids_beyond_tors(
            std::unordered_set<int>) = 0;
};

class TwoLayers: public Hierarchy {
public:
    TwoLayers(JobDispatcher *job_dispatcher) :
            job_dispatcher(job_dispatcher) {
    }
    void process_hierarchy_query(HierarchyQuery*) override;
    void setup_job(Job*, const std::unordered_map<int, unsigned>&) override;
    std::unordered_set<int> switch_ids_beyond_tors(std::unordered_set<int>)
            override;
private:
    Switch *core_switch;
    int core_switch_id;
    JobDispatcher *job_dispatcher;
};

#endif /* HIERARCHY_H_ */
