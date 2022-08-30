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

class JobDispatcher: public cSimpleModule {
private:
    unsigned n_workers;
    unsigned switch_ports;
    std::map<uint64_t, Job*> jobs { }; // jid->job
    std::vector<Worker*> workers { };
    std::unordered_map<uint64_t, unsigned> gpu_count { };

    typedef Job* (JobDispatcher::*pick_func)(void);
    pick_func pick_a_job_to_execute;
    Job* fifo();

    typedef std::unordered_map<Worker*, unsigned> (JobDispatcher::*place_func)(
            Job*);
    place_func get_placement;

    bool force_distributed { false };
    bool force_multi_racks { false };
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
        std::vector<T> res;
        for (auto i : res_index) {
            res.push_back(vec[i]);
        }
        return res;
    }
    std::unordered_map<Worker*, unsigned> random(Job*);

    typedef void (JobDispatcher::*setup_func)(uint64_t,
            const std::unordered_map<Worker*, unsigned>&);
    setup_func setup_switches;
    void two_layers(uint64_t, const std::unordered_map<Worker*, unsigned>&);

protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
};

#endif /* JOBDISPATCHER_H_ */
