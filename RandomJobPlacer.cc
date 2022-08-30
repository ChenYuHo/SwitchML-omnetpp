#include "SwitchML_m.h"
#include "Worker.h"
#include <omnetpp.h>
#include <deque>
#include <unordered_map>
using namespace omnetpp;

class RandomJobPlacer: public cSimpleModule {
private:
    unsigned n_workers;
    std::vector<Job*> jobs { };
    std::vector<Worker*> workers { };
    std::unordered_map<uint64_t, unsigned> gpu_count { };
    Job* pick_job();
    template<typename T> std::vector<T> sample(std::vector<T> vec,
            size_t size) {
        std::vector<size_t> res_index(size);
        auto max_size = vec.size();
        for (size_t i = 0; i < max_size; ++i) {
            size_t j = intuniform(0, i);
            if (j < size) {
                if (i < size) {
                    res_index[i] = res_index[j];
                }
                res_index[j] = i;
            }
        }

        std::vector<T> res(size);
        for (auto i : res_index) {
            res.push_back(vec[i]);
        }
        return res;
    }
protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
};

Define_Module(RandomJobPlacer);

void RandomJobPlacer::initialize() {
    n_workers = getParentModule()->par("n_workers");
    workers.reserve(n_workers);
    for (unsigned i = 0; i < n_workers; ++i) {
        workers.push_back(
                (Worker*) getParentModule()->getSubmodule("workers", i));
    }

}

Job* RandomJobPlacer::pick_job() {
    if (jobs.empty())
        return nullptr;
    return jobs[0];
}

void RandomJobPlacer::handleMessage(cMessage *msg) {
// ask for a job to place (msg can come from submitter, or when job finishes)
    auto job = pick_job();
    if (job == nullptr) {
        // no available jobs;
        delete msg;
        return;
    }
    unsigned available_machines = 0;
//    std::unordered_set<unsigned> tors_with_available_machines;
    for (auto worker : workers) {
        if (worker->get_free_gpus() > 0) {
            available_machines++;
//            tors_with_available_machines.insert(machine->tor->id);
        }
//        for (int i = 0; i < machine->gpu; i++)
//            candidates.push_back(machine);
    }

    auto a = sample(std::vector<unsigned> { 1, 2, 3, 4, 4 }, 3);
}

