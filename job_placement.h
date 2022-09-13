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
    bool force_single_rack;
    bool fall_back_to_random;
    bool multi_racks_placement(const std::vector<int> &selected) {
        std::unordered_set<int> placed_tors { };
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

    bool single_rack_placement(const std::vector<int> &selected) {
        std::unordered_set<int> placed_tors { };
        std::unordered_set<int> workers { };
        for (const auto wid : selected) {
            placed_tors.insert(job_dispatcher->tor_id_for_worker[wid]);
            workers.insert(wid);
        }
        return workers.size() > 1 && placed_tors.size() == 1;
    }

public:
    Random(cRNG *rng, JobDispatcher *job_dispatcher, bool distributed = false,
            bool multi_racks = false, bool fall_back = true) :
            rng(rng), job_dispatcher(job_dispatcher), fall_back_to_random(
                    fall_back) {
        force_distributed = distributed && !multi_racks;
        force_single_rack = !distributed && multi_racks;
        force_multi_racks = distributed && multi_racks;
    }

    template<typename T> const std::vector<T> sample(const std::vector<T> &vec,
            int size) {
        if (vec.size() == size_t(size)) {
            return vec;
        }
        std::vector<int> res_index(size);
        int max_size = vec.size();
        for (int i = 0; i < max_size; ++i) {
            int j = omnetpp::intuniform(rng, 0, i);
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
        std::unordered_set<unsigned> tors_with_available_machines { };
        std::unordered_map<int, unsigned> free_gpus_in_tor { };
        for (auto &pair : job_dispatcher->workers) {
            auto wid = pair.first;
            if (job_dispatcher->free_gpus[wid] > 0) {
                available_machines++;
                auto tor_id = job_dispatcher->tor_id_for_worker[wid];
                tors_with_available_machines.insert(tor_id);
                free_gpus_in_tor[tor_id] += job_dispatcher->free_gpus[wid];
            }
            for (unsigned i = 0; i < job_dispatcher->free_gpus[wid]; ++i) {
                candidates.push_back(wid);
            }
        }

        if (job->getGpu() > candidates.size()) {
            return counter; // not enough free GPUs
        } else if (job->getGpu() == 1) {
            // just choose one, nothing more to do
            for (auto wid : sample(candidates, 1)) {
                counter[wid] += 1;
            }
            return counter;
        }

        bool can_place_within_single_rack = false;
        for (auto &pair : free_gpus_in_tor) {
            if (pair.second >= job->getGpu()) {
                can_place_within_single_rack = true;
            }
        }
        bool can_place_distributed = available_machines > 1;
        bool can_place_multi_racks = tors_with_available_machines.size() > 1;

        bool can_satisfy_requirement = (force_single_rack
                && can_place_within_single_rack)
                || (force_distributed && can_place_distributed)
                || (force_multi_racks && can_place_multi_racks);

        if (!can_satisfy_requirement) {
            if (!fall_back_to_random) {
                return counter;
            } else {
                for (auto wid : sample(candidates, 1)) {
                    counter[wid] += 1;
                }
                return counter;
            }
        }

        // at this point, must satisfy one of the three requirements
        std::vector<int> selected;
        if (force_single_rack) {
            do {
                selected = sample(candidates, job->getGpu());
            } while (!single_rack_placement(selected));
        } else if (force_distributed) {
            do {
                selected = sample(candidates, job->getGpu());
            } while (!distributed_placement(selected));
        } else if (force_multi_racks) {
            do {
                selected = sample(candidates, job->getGpu());
            } while (!multi_racks_placement(selected));
        }
        for (auto wid : selected) {
            counter[wid] += 1;
        }
        if (!getEnvir()->isLoggingEnabled()) {
            EV_DEBUG << "\nPlacement:\n";
            for (auto pair : counter) {
                EV_DEBUG << pair.first << "->" << pair.second << " ";
            }
            EV_DEBUG << endl;
        }
        return counter;
    }
}
;

#endif /* JOB_PLACEMENT_H_ */
