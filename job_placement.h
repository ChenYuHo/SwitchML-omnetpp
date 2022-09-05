#ifndef JOB_PLACEMENT_H_
#define JOB_PLACEMENT_H_
#include <omnetpp.h>
#include "JobDispatcher.h"

class JobPlacement {
public:
    virtual std::unordered_map<int, unsigned> place_job(const Job*) = 0;
    virtual ~JobPlacement() = default;
};

class Random: public JobPlacement {
private:
    cRNG *rng;
    JobDispatcher *job_dispatcher;
    bool force_distributed;
    bool force_multi_racks;
    bool multi_racks_placement(const std::vector<int> &selected) {
        std::unordered_set<unsigned> placed_tors;
        for (const auto wid : selected) {
            placed_tors.insert(job_dispatcher->tor_id_for_worker[wid]);
        }
        return placed_tors.size() > 1;
    }

    bool distributed_placement(const std::vector<int> &selected) {
        std::unordered_set<int> workers { };
        for (const auto wid : selected) {
            workers.insert(wid);
        }
        return workers.size() > 1;
    }

public:
    Random(cRNG *rng, JobDispatcher *job_dispatcher, bool force_distributed =
            false, bool force_multi_racks = false) :
            rng(rng), job_dispatcher(job_dispatcher), force_distributed(
                    force_distributed), force_multi_racks(force_multi_racks) {
    }

    template<typename T> std::vector<T> sample(std::vector<T> vec,
            size_t size) {
        std::vector<size_t> res_index(size);
        auto max_size = vec.size();
        for (size_t i = 0; i < max_size; ++i) {
            size_t j = omnetpp::intuniform(rng, 0, i);
            if (j < size) {
                if (i < size) {
                    res_index[i] = res_index[j];
                }
                res_index[j] = i;
            }
        }
        std::vector<T> res;
        for (auto i : res_index) {
            res.push_back(vec[i]);
        }
        return res;
    }

    std::unordered_map<int, unsigned> place_job(const Job *job) override {
        std::vector<int> candidates;
        std::unordered_map<int, unsigned> counter { };
        unsigned available_machines = 0;
        std::unordered_set<unsigned> tors_with_available_machines;
        for (auto pair : job_dispatcher->workers) {
            auto wid = pair.first;
            if (job_dispatcher->free_gpus[wid] > 0) {
                available_machines++;
                tors_with_available_machines.insert(
                        job_dispatcher->tor_id_for_worker[wid]);
            }
            for (int i = 0; i < job_dispatcher->free_gpus[wid]; ++i)
                candidates.push_back(wid);
        }

        if (job->getGpu() > candidates.size())
            return counter; // not enough free GPUs
        std::vector<int> selected;
        auto must_place_distributed = force_distributed
                && available_machines > 1 && job->getGpu() > 1;
        auto must_place_multi_racks = force_multi_racks
                && tors_with_available_machines.size() > 1 && job->getGpu() > 1;
        do {
            do {
                selected = sample(candidates, job->getGpu());
            } while (must_place_distributed && !distributed_placement(selected));
        } while (must_place_multi_racks && !multi_racks_placement(selected));
        for (auto wid : selected) {
            counter[wid] += 1;
        }
        return counter;
    }
};

#endif /* JOB_PLACEMENT_H_ */
