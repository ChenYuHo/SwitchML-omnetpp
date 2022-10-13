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
    int placement_type;
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
    Random(cRNG *rng, JobDispatcher *job_dispatcher, int placement_type = 0) :
            rng(rng), job_dispatcher(job_dispatcher), placement_type(
                    placement_type) {
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
        std::vector<int> candidates { };
        std::vector<int> selected { };
        std::unordered_map<int, unsigned> counter { };
        unsigned num_gpus_needed = job->getGpu();

        for (const auto &pair : job_dispatcher->workers) {
            auto wid = pair.first;
            for (unsigned i = 0; i < job_dispatcher->free_gpus[wid]; ++i)
                candidates.push_back(wid);
        }

        if (num_gpus_needed > candidates.size()) {
            return counter; // not enough free GPUs
        }

        if (num_gpus_needed == 1) {
            // just choose one, nothing more to do
            counter[candidates[omnetpp::intuniform(rng, 0,
                    candidates.size() - 1)]] += 1;
            return counter;
        }

        unsigned num_machines_with_free_gpus = 0;
        for (const auto &pair : job_dispatcher->workers) {
            auto wid = pair.first;
            if (job_dispatcher->free_gpus[wid] > 0) {
                num_machines_with_free_gpus++;
            }
        }

        switch (placement_type) {
        case 3: {
            unsigned num_machines_with_free_gpus = 0;
            std::unordered_set<int> tors_with_available_machines { };
            for (const auto &pair : job_dispatcher->workers) {
                auto wid = pair.first;
                if (job_dispatcher->free_gpus[wid] > 0) {
                    num_machines_with_free_gpus++;
                    tors_with_available_machines.insert(
                            job_dispatcher->tor_id_for_worker[wid]);
                }
            }
            if (tors_with_available_machines.size() > 1) { // must place multirack
                do {
                    selected = sample(candidates, num_gpus_needed);
                } while (!multi_racks_placement(selected));
            }
            break;
        }
        case 2: { // single rack
            std::unordered_map<int, unsigned> num_machines_with_free_gpus_in_tor { };
            std::unordered_map<int, unsigned> free_gpus_in_tor { }; // tor_id -> free gpus
            std::unordered_map<int, std::vector<int>> candidates_for_tor { };
            for (const auto &pair : job_dispatcher->workers) {
                auto wid = pair.first;
                auto tor_id = job_dispatcher->tor_id_for_worker[wid];
                auto worker_free_gpus = job_dispatcher->free_gpus[wid];
                if (worker_free_gpus > 0) {
                    num_machines_with_free_gpus_in_tor[tor_id] += 1;
                    free_gpus_in_tor[tor_id] += worker_free_gpus;
                }
                for (unsigned i = 0; i < worker_free_gpus; ++i)
                    candidates_for_tor[tor_id].push_back(wid);
            }

            // find if single rack (distributed) placement if possible
            bool can_place_single_rack = false;
            std::vector<int> tor_candidates;
            for (const auto &pair : free_gpus_in_tor) {
                if (pair.second > num_gpus_needed
                        && num_machines_with_free_gpus_in_tor[pair.first] > 1) {
                    can_place_single_rack = true;
                    for (unsigned i = 0; i < pair.second; ++i) {
                        tor_candidates.push_back(pair.first);
                    }
                }
            }

            if (can_place_single_rack) {
                // choose rack
                auto selected_tor = tor_candidates[omnetpp::intuniform(rng, 0,
                        tor_candidates.size() - 1)];
                const auto &candidate_workers = candidates_for_tor[selected_tor];
                do {
                    selected = sample(candidate_workers, num_gpus_needed);
                } while (!distributed_placement(selected));
            } // else fall back to distributed ?
            break;
        }
        case 1: { // distributed
            if (num_machines_with_free_gpus > 1) { // must place distributed
                do {
                    selected = sample(candidates, num_gpus_needed);
                } while (!distributed_placement(selected));
            }
            break;
        }
        case 0:
        default:
            selected = sample(candidates, num_gpus_needed);
            break;
        }

        for (auto wid : selected) {
            counter[wid] += 1;
        }
        if (getEnvir()->isLoggingEnabled() && !counter.empty()) {
            EV_DEBUG << "\nPlacement:\n";
            for (auto pair : counter) {
                EV_DEBUG << pair.first << "->" << pair.second << " ";
            }
            EV_DEBUG << endl;
        }
        return counter;
    }
};

#endif /* JOB_PLACEMENT_H_ */
