#include "SwitchML_m.h"
#include "JobDispatcher.h"
#define FMT_HEADER_ONLY
#include "fmt/format.h"
#include "Hierarchy.h"
#include "JobScheduling.h"
#include "JobPlacement.h"
#include "Switch.h"

using namespace omnetpp;

Define_Module(JobDispatcher);

void JobDispatcher::initialize(int stage) {
    if (stage == 0) {
        collective_scheduler = getSimulation()->findModuleByPath(
                "<root>.collective_scheduler");
        if (collective_scheduler) {
            EV_INFO << "Collective Scheduler is "
                           << collective_scheduler->getFullName() << endl;
        } else {
            EV_INFO << "No Collective Scheduler" << endl;
        }
        switch_ports = getParentModule()->par("switch_ports");
        n_workers = getParentModule()->par("n_workers");
        std::string h = par("hierarchy");
        if (h == "two_layers") {
            hierarchy = new TwoLayers(this);
        } else {
            EV_FATAL << "Unexpected hierarchy: " << h << endl;
        }

        std::string js = par("job_scheduling");
        if (js == "fifo") {
            job_scheduling = new Fifo();
        } else {
            EV_FATAL << "Unexpected Job Scheduling: " << js << endl;
        }

        jobSubmissionTime = registerSignal("jobSubmissionTime");
        jobCompletionTime = registerSignal("jobCompletionTime");
        jobStartTime = registerSignal("jobStartTime");
        jobWaitTime = registerSignal("jobWaitTime");
        jobPlacementType = registerSignal("jobPlacementType");
    } else if (stage == 1) {
        for (int i = 0; i < n_workers; ++i) {
            auto wid = getParentModule()->findSubmodule("workers", i);
            index_to_wid[i] = wid;
            auto worker = (Worker*) getSimulation()->getModule(wid);
            auto tor_id = worker->tor_id();
            tor_id_for_worker[wid] = tor_id;
            workers_for_tor[tor_id].push_back(wid);
            workers[wid] = worker;
            free_gpus[wid] = worker->par("num_gpus");
        }
        std::string p = par("job_placement");
        if (p == "random") {
            job_placement = new Random(getRNG(0), this, 0);
        } else if (p == "random_distributed") { // single or multi rack
            job_placement = new Random(getRNG(0), this, 1);
        } else if (p == "random_singlerack") {
            job_placement = new Random(getRNG(0), this, 2);
        } else if (p == "random_multiracks") {
            job_placement = new Random(getRNG(0), this, 3);
        } else if (p == "random_distributed_fallback") { // single or multi rack
            job_placement = new Random(getRNG(0), this, 4);
        } else if (p == "random_singlerack_fallback") {
            job_placement = new Random(getRNG(0), this, 5);
        } else if (p == "random_multiracks_fallback") {
            job_placement = new Random(getRNG(0), this, 6);
        } else if (p == "two_jobs") {
            job_placement = new Random(getRNG(0), this, 7);
        } else if (p == "custom") {
            job_placement = new Custom(this,
                    par("custom_placement").stdstringValue());
        } else {
            EV_FATAL << "Unexpected Job Placement: " << p << endl;
        }
        sendDirect(new cMessage,
                getParentModule()->getSubmodule("job_submitter"), "directin");

//        listener = new MyListener;
//        this->getSimulation()->getSystemModule()->subscribe("iterTime",
//                listener);

    }

}

struct DoubleDefaultedToOne {
    double d = 1;
};

void JobDispatcher::bssi(std::deque<TensorKey> &result,
        std::unordered_map<TensorKey, double> weights,
        const std::unordered_map<TensorKey, uint64_t> &remaining_sizes) {
    auto iters = weights.size();
#ifndef NDEBUG
    EV_DEBUG << "bssi on:\n";
    for (auto &pair : weights) {
        EV_DEBUG << " jid " << pair.first.job_id << " layer "
                        << pair.first.layer << " weight " << pair.second
                        << endl;
    }
    std::stringstream ss;
    ss << "bssi result (last->first):";
#endif
    for (unsigned i = 0; i < iters - 1; ++i) {
        std::unordered_map<int, // worker id
                std::unordered_map<uint64_t, DoubleDefaultedToOne>> data_port_coflow { };
        // port (per worker), coflow -> data
        std::unordered_map<int, DoubleDefaultedToOne> data_port { };
        // Find the most bottlenecked port
        int bottlenecked; // port id, every worker has one port, so use worker id as port id
        double current_max = 0;
        for (auto &pair : weights) { // tensor_key -> weight
            auto &tensor_key = pair.first;
            auto jid = tensor_key.job_id;
            for (auto wid : workers_for_job[jid]) {
                auto data = remaining_sizes.at(tensor_key);
                data_port_coflow[wid][jid].d += data;
                data_port[wid].d += data;
                if (data_port[wid].d >= current_max) {
                    current_max = data_port[wid].d;
                    bottlenecked = wid;
                }
            }
        }
        EV_DEBUG << "bottlenecked port " << bottlenecked << endl;
        // Select weighted largest job to schedule last
        TensorKey weighted_largest;
        auto current_min = DBL_MAX;
        double min_weight = DBL_MAX;
        for (auto &pair : weights) { // tensor_key -> weight
            // scaled weight
            auto weight = pair.second
                    / data_port_coflow[bottlenecked][pair.first.job_id].d;
            if (weight <= current_min) {
                current_min = weight;
                weighted_largest.layer = pair.first.layer;
                weighted_largest.job_id = pair.first.job_id;
                min_weight = pair.second;
            }
        }
        // Scale the weights
        auto s = data_port_coflow[bottlenecked][weighted_largest.job_id].d;
        for (auto &pair : weights) {
            pair.second -= (min_weight
                    * data_port_coflow[bottlenecked][pair.first.job_id].d / s);
        }
#ifndef NDEBUG
        ss << " jid " << weighted_largest.job_id << " layer "
                << weighted_largest.layer << " weight " << current_min << " ->";
#endif
        result.push_front(weighted_largest);
        weights.erase(weighted_largest);
    }
    // final one left
    auto &tensor_key = weights.begin()->first;
    result.push_front(tensor_key);
#ifndef NDEBUG
    ss << " jid " << tensor_key.job_id << " layer " << tensor_key.layer << endl;
    EV_DEBUG << ss.str();
#endif
}

void JobDispatcher::clean_resources_for_tensor_key(uint64_t jid,
        const TensorKey &tensor_key) {
    for (auto tor_id : switches_for_job[jid]) {
        ((Switch*) (getSimulation()->getModule(tor_id)))->clean_resources_for_tensor(
                tensor_key);
    }
}

bool JobDispatcher::accommodate(
        const std::unordered_map<TensorKey, unsigned> &num_workers_of_active_tensor_key,
        uint64_t jid_to_add, bool exclusive) {
    auto active_switch_ids = std::unordered_set<int> { };
    for (auto &pair : num_workers_of_active_tensor_key) {
        auto jid = pair.first.job_id;
        for (auto switch_id : switches_for_job[jid]) {
            active_switch_ids.insert(switch_id);
        }
    }
    for (auto switch_id : switches_for_job[jid_to_add]) {
        if (active_switch_ids.find(switch_id) != active_switch_ids.end()) {
            // found
            return false;
        } else if (!exclusive) {
            // idle switch found
            return true;
        }
    }
    // none found
    return true;
}

//bool JobDispatcher::accommodate(
//        const std::unordered_set<uint64_t> &existing_jids,
//        uint64_t jid_to_add) {
//    auto active_switch_ids = std::unordered_set<int> { };
//    for (auto jid : existing_jids) {
//        for (auto switch_id : switches_for_job[jid]) {
//            active_switch_ids.insert(switch_id);
//        }
//    }
//    for (auto switch_id : switches_for_job[jid_to_add]) {
//        if (active_switch_ids.find(switch_id) != active_switch_ids.end()) {
//            // found
//            return false;
//        }
//    }
//    return true;
//}

bool JobDispatcher::tryDispatchAJob() {
    auto job = job_scheduling->pick_a_job_to_execute(jobs);
    if (!job) {
        EV_DEBUG << "[JobDispatcher]\t" << simTime()
                        << "\tcan't pick a job to execute" << endl;
        return false;
    } else {
        EV_DEBUG << "[JobDispatcher]\t" << simTime()
                        << "\tselects and tries to place job "
                        << job->getJob_id() << endl;
    }
    auto placement = job_placement->place_job(job);
    if (placement.empty()) {
        EV_DEBUG << "[JobDispatcher]\t" << simTime()
                        << "\tcan't satisfy placement" << endl;
        return false;
    }

    {
        auto &workers_of_job = workers_for_job[job->getJob_id()];
        auto &switches = switches_for_job[job->getJob_id()];
        for (auto &pair : placement) {
            EV_DEBUG << "wid " << pair.first << " num_gpus " << pair.second
                            << endl;
            workers_of_job.insert(pair.first);
            auto tor_id = tor_id_for_worker[pair.first];
            switches.insert(tor_id);
        }
        auto beyond_tor = hierarchy->switch_ids_beyond_tors(switches);
        for (auto switch_id : beyond_tor) {
            switches.insert(switch_id);
        }
        if (workers_of_job.size() == 1) {
            emit(jobPlacementType, 1); // single machine
        } else if (workers_of_job.size() > 1 && switches.size() == 1) { // distributed single-rack
            emit(jobPlacementType, 2);
        } else { // workers_of_job.size() > 1 && switches.size() > 1 {// distrubted multi-racks
            emit(jobPlacementType, 3);
        }
    }

    job->setNum_workers_allocated(placement.size());
    job->setStart_time(simTime());
    emit(jobStartTime, simTime());
    EV_DEBUG << "[JobDispatcher]\t" << simTime() << "\tstarting job "
                    << job->getJob_id() << " at " << simTime()
                    << " submitted at " << job->getSubmit_time() << endl;
    emit(jobWaitTime, simTime() - job->getSubmit_time());
    unsigned rank = 0;
    hierarchy->setup_job(job, placement);
    for (auto pair : placement) {
        auto wid = pair.first;
        auto gpus = pair.second;
        free_gpus[wid] -= gpus;
        auto dup = job->dup();
        dup->setKind(3); // for workers to identify new job arrival
        // local copy job uses kind as number of workers that finished the job
        dup->setGpu(gpus);
        dup->setRank(rank++);
        sendDirect(dup, workers[wid], "directin");
    }
    return true;
}

void JobDispatcher::handleMessage(cMessage *msg) {
    auto job = (Job*) msg;
    if (job->getFinish_time() < 0) {
        // this is a newly submitted job
        EV_DEBUG << "[JobDispatcher]\t" << simTime()
                        << "\tReceived submitted job " << job->getJob_id()
                        << " requiring " << job->getGpu() << " GPUS" << endl;
        job->setSubmit_time(simTime());
        jobs[job->getJob_id()] = job; // saved as a local copy, don't delete
        job->setKind(0); // use kind as number of workers that finished the job
        emit(jobSubmissionTime, job->getSubmit_time());
        while (tryDispatchAJob()) {
            // send jobs until nothing left or nothing can be placed
        }
    } else { // a worker reports a finished job
        auto local_copy = jobs[job->getJob_id()];
        short num_received = local_copy->getKind() + 1;
        local_copy->setKind(num_received);
        // worker sets kind using its id
        free_gpus[job->getWorker_id()] += job->getGpu();
        if (uint32_t(local_copy->getKind())
                == local_copy->getNum_workers_allocated()) {
            // all workers finished
            EV_DEBUG << "Finished job " << job->getJob_id() << " at "
                            << simTime() << endl;
            emit(jobCompletionTime, simTime() - job->getStart_time());

            auto jid = job->getJob_id();
            for (auto tor_id : switches_for_job[jid]) {
                ((Switch*) (getSimulation()->getModule(tor_id)))->clean_resources_for_job(
                        jid);
            }
            if (collective_scheduler) {
                // kind 5
                sendDirect(job->dup(), collective_scheduler, "directin");
            }

            delete local_copy;
            jobs.erase(jid);

            while (tryDispatchAJob()) {
                // send jobs until nothing left or nothing can be placed
            }
        }
        delete msg;
    }
}

JobDispatcher::~JobDispatcher() {
    // clean local copies
    for (auto &pair : jobs) {
        delete pair.second;
    }
    jobs.clear();
    delete job_scheduling;
    delete job_placement;
    delete hierarchy;
}
