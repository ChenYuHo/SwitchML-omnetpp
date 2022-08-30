#include "SwitchML_m.h"
#include "JobDispatcher.h"
#define FMT_HEADER_ONLY
#include "fmt/format.h"

using namespace omnetpp;

Define_Module(JobDispatcher);

void JobDispatcher::initialize() {
    pick_a_job_to_execute = &JobDispatcher::fifo;
    get_placement = &JobDispatcher::random;
    setup_switches = &JobDispatcher::two_layers;
    switch_ports = getParentModule()->par("switch_ports");
    n_workers = getParentModule()->par("n_workers");
    workers.reserve(n_workers);
    for (unsigned i = 0; i < n_workers; ++i) {
        auto worker = (Worker*) getParentModule()->getSubmodule("workers", i);
        workers.push_back(worker);
    }
}

Job* JobDispatcher::fifo() {
    if (jobs.empty())
        return nullptr;
    return jobs.begin()->second; // already sorted
}

bool multi_racks_placement(const std::vector<Worker*> &selected) {
    std::unordered_set<unsigned> placed_tors;
    for (const auto &worker : selected) {
        placed_tors.insert(worker->get_tor_id());
    }
    return placed_tors.size() > 1;
}

bool distributed_placement(const std::vector<Worker*> &selected) {
    std::unordered_set<unsigned> workers { };
    for (const auto &worker : selected) {
        workers.insert(worker->getId());
    }
    return workers.size() > 1;
}

std::unordered_map<Worker*, unsigned> JobDispatcher::random(Job *job) {
    std::vector<Worker*> candidates;
    std::unordered_map<Worker*, unsigned> counter { };
    unsigned available_machines = 0;
    std::unordered_set<unsigned> tors_with_available_machines;
    for (auto worker : workers) {
        if (worker->get_free_gpus() > 0) {
            available_machines++;
            tors_with_available_machines.insert(worker->get_tor_id());
        }
        for (int i = 0; i < worker->get_free_gpus(); ++i)
            candidates.push_back(worker);
    }

    if (job->getGpu() > candidates.size())
        return counter; // not enough free GPUs
    std::vector<Worker*> selected;
    auto must_place_distributed = force_distributed && available_machines > 1
            && job->getGpu() > 1;
    auto must_place_multi_racks = force_multi_racks
            && tors_with_available_machines.size() > 1 && job->getGpu() > 1;
    do {
        do {
            selected = sample(candidates, job->getGpu());
        } while (must_place_distributed && !distributed_placement(selected));
    } while (must_place_multi_racks && !multi_racks_placement(selected));
    for (auto worker : selected) {
        counter[worker] += 1;
    }
    return counter;
}

void JobDispatcher::two_layers(uint64_t job_id,
        const std::unordered_map<Worker*, unsigned> &placement) {
    std::unordered_map<int, uint64_t> num_updates { };
    for (auto &pair : placement) {
        auto worker = pair.first;
        num_updates[worker->get_tor_id()] += 1;
    }
    for (auto &pair: num_updates) {
        auto switch_id = pair.first;
        auto tor = (Switch*)this->getSimulation()->getModule(switch_id);
        tor->set_num_updates_for_job(job_id, pair.second);
        EV << fmt::format("jid {} set switch {} num_updates {}", job_id, switch_id, pair.second);
    }
    auto core = (Switch*) getParentModule()->getSubmodule("core");
    core->set_num_updates_for_job(job_id, num_updates.size());
    EV << fmt::format("jid {} set core switch num_updates {}", job_id, num_updates.size());

    core->set_top_level_for_job(job_id, true);
    for (unsigned i = 0; i < switch_ports; ++i) {
        auto tor = (Switch*) getParentModule()->getSubmodule("tors", i);
        tor->set_top_level_for_job(job_id, false);
    }
}

void JobDispatcher::handleMessage(cMessage *msg) {
    auto job = check_and_cast<Job*>(msg);
    if (job->getFinish_time() == 0) {
        // this is a newly submitted job
        EV << "Received submitted job " << job->getJob_id() << endl;
        jobs[job->getJob_id()] = job;
    } else { // this is a finished job
        EV << "Finished job " << job->getJob_id() << endl;
        jobs.erase(job->getJob_id());
        delete job;
    }

    auto next_job = (this->*pick_a_job_to_execute)();
    if (!next_job) {
        return;
    }

    auto workers_to_execute_job_on = (this->*get_placement)(job);

    job->setNum_workers_allocated(workers_to_execute_job_on.size());
    job->setStart_time(simTime());
    unsigned rank = 0;
    (this->*setup_switches)(job->getJob_id(), workers_to_execute_job_on);
    for (auto pair : workers_to_execute_job_on) {
        auto worker = pair.first;
        auto gpus = pair.second;
        EV << "Use " << gpus << " GPUs of Worker " << worker->getId() << endl;

        auto dup = job->dup();
        dup->setGpu(gpus);
        dup->setRank(rank++);
        sendDirect(dup, worker, "jobin");
    }

//    auto a = sample(std::vector<unsigned> { 1, 2, 3, 4, 4 }, 3);
}
