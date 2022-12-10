#ifndef JOBPLACEMENT_H_
#define JOBPLACEMENT_H_
#include <omnetpp.h>
#include "JobDispatcher.h"

class JobPlacement {
public:
    virtual std::unordered_map<int, unsigned> place_job(const Job*) = 0;
    virtual ~JobPlacement() = default;
};

class Custom: public JobPlacement {
private:
    std::string placement_str;
    std::unordered_map<uint64_t, std::unordered_map<int, unsigned>> placement { };
public:
    Custom(JobDispatcher *job_dispatcher, std::string placement_str) :
            placement_str(placement_str) {
        // example: 5-0&2,4-1:4&3:2,7-3
        // comma separates placement_str as job_placement, whose format is JID-PLACEMENT
        // & separates PLACEMENT as worker_gpus, whose format is WID:NUM_GPUS where NUM_GPUS defaults to 1 if no ":" exists
        std::stringstream ss(placement_str);
        std::string job_placement;
        while (getline(ss, job_placement, ',')) {
            auto idx = job_placement.find("-");
            if (idx == std::string::npos) {
                std::cerr << "placement string format error" << endl;
                exit(1);
            }
            auto jid = std::stoull(job_placement.substr(0, idx));
            std::stringstream sss(job_placement.substr(idx + 1));
            std::string worker_gpus;
            while (getline(sss, worker_gpus, '&')) {
                auto idx = worker_gpus.find(":");
                if (idx == std::string::npos) {
                    auto wid = job_dispatcher->index_to_wid[std::stoi(
                            worker_gpus)];
                    EV_DEBUG << "index " << worker_gpus << " wid " << wid
                                    << " num_gpus 1" << endl;
                    placement[jid][wid] = 1;
                } else {
                    auto wid = job_dispatcher->index_to_wid[std::stoi(
                            worker_gpus.substr(0, idx))];
                    EV_DEBUG << "index " << worker_gpus.substr(0, idx)
                                    << " wid " << wid << " num_gpus "
                                    << worker_gpus.substr(idx + 1) << endl;
                    placement[jid][wid] = std::stoul(
                            worker_gpus.substr(idx + 1));
                }
            }
        }
        for (auto &pair : placement) {
            EV_DEBUG << "JID " << pair.first << " placement:";
            for (auto p : pair.second) {
                EV_DEBUG << "wid " << p.first << " gpu " << p.second << " ";
            }
            EV_DEBUG << endl;
        }

    }
    ;
    std::unordered_map<int, unsigned> place_job(const Job *job) override {
        return placement[job->getJob_id()];
    }
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
        std::unordered_map<int, unsigned> counter { };
        if (placement_type == 7) {
            auto &pair = *(job_dispatcher->workers_for_tor.begin());
            for (auto wid : pair.second) {
                counter[wid] = 1;
            }
            return counter;
        }

        std::vector<int> candidates { };
        std::vector<int> selected { };
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
        case 6:
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
                break;
            } else if (placement_type == 3) {
                // no placement found
                break;
            } // try place single rack
            EV_DEBUG << "[JobPlacement]\t" << simTime() << "\tcan't place multi rack, try single rack" << endl;
        }
        case 5:
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
                break;
            } else if (placement_type == 2) {
                // no placement found
                break;
            } // try distributed
            EV_DEBUG << "[JobPlacement]\t" << simTime() << "\tcan't place single rack, try distributed" << endl;
        }
        case 4:
        case 1: { // distributed
            if (num_machines_with_free_gpus > 1) { // must place distributed
                do {
                    selected = sample(candidates, num_gpus_needed);
                } while (!distributed_placement(selected));
                break;
            } else if (placement_type == 1) {
                // no placement found
                break;
            } // fallback to random selection
            EV_DEBUG << "[JobPlacement]\t" << simTime() << "\tfallback to random selection" << endl;
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
            EV_DEBUG << "[JobPlacement]\t" << simTime() << "\tPlacement: ";
            for (auto pair : counter) {
                EV_DEBUG << pair.first << "->" << pair.second << " ";
            }
            EV_DEBUG << endl;
        }
        return counter;
    }
};

#endif /* JOBPLACEMENT_H_ */
