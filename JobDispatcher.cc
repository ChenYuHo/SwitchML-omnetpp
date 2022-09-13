#include "SwitchML_m.h"
#include "JobDispatcher.h"
#define FMT_HEADER_ONLY
#include "fmt/format.h"
#include "hierarchy.h"
#include "job_scheduling.h"
#include "job_placement.h"

using namespace omnetpp;

Define_Module(JobDispatcher);

void JobDispatcher::initialize(int stage) {
    if (stage == 0) {
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

        std::string p = par("job_placement");
        if (p == "random") {
            job_placement = new Random(getRNG(0), this, false, false, true);
        } else if (p == "random_distributed") {
            job_placement = new Random(getRNG(0), this, true, false, false);
        } else if (p == "random_singlerack") {
            job_placement = new Random(getRNG(0), this, false, true, false);
        } else if (p == "random_multiracks") {
            job_placement = new Random(getRNG(0), this, true, true, false);
        } else {
            EV_FATAL << "Unexpected Job Placement: " << p << endl;
        }

        jsmtSignal = registerSignal("jobSubmissionTime");
        jctSignal = registerSignal("jobCompletionTime");
        jstSignal = registerSignal("jobStartTime");
        jwtSignal = registerSignal("jobWaitTime");
        jpSignal = registerSignal("jobPlacementType");
    } else if (stage == 1) {
        for (unsigned i = 0; i < n_workers; ++i) {
            auto wid = getParentModule()->findSubmodule("workers", i);
            auto worker = (Worker*) getSimulation()->getModule(wid);
            tor_id_for_worker[wid] = worker->tor_id();
            workers[wid] = worker;
            free_gpus[wid] = worker->par("num_gpus");
        }
        sendDirect(new cMessage,
                getParentModule()->getSubmodule("job_submitter"), "directin");
    }

}

bool JobDispatcher::accommodate(
        unordered_map<uint64_t, unsigned> num_workers_of_active_job_id,
        uint64_t jid_to_add) {
    auto active_switch_ids = std::unordered_set<int> { };
    for (auto &pair : num_workers_of_active_job_id) {
        auto jid = pair.first;
        for (auto switch_id : switches_for_job[jid]) {
            active_switch_ids.insert(switch_id);
        }
    }
    for (auto switch_id : switches_for_job[jid_to_add]) {
        if (active_switch_ids.find(switch_id) != active_switch_ids.end()) {
            // found
            return false;
        }
    }
    return true;
}

bool JobDispatcher::accommodate(unordered_set<uint64_t> existing_jids,
        uint64_t jid_to_add) {
    auto active_switch_ids = std::unordered_set<int> { };
    for (auto jid : existing_jids) {
        for (auto switch_id : switches_for_job[jid]) {
            active_switch_ids.insert(switch_id);
        }
    }
    for (auto switch_id : switches_for_job[jid_to_add]) {
        if (active_switch_ids.find(switch_id) != active_switch_ids.end()) {
            // found
            return false;
        }
    }
    return true;
}

bool JobDispatcher::tryDispatchAJob() {
    auto job = job_scheduling->pick_a_job_to_execute(jobs);
    if (!job) {
        return false;
    }

    auto placement = job_placement->place_job(job);

    if (placement.empty()) {
        // can't satisfy placement
        return false;
    }

    {
        auto &workers = workers_for_job[job->getJob_id()];
        auto &switches = switches_for_job[job->getJob_id()];
        for (auto &pair : placement) {
            workers.insert(pair.first);
            auto tor_id = tor_id_for_worker[pair.first];
            switches.insert(tor_id);
        }
        auto beyond_tor = hierarchy->switch_ids_beyond_tors(switches);
        for (auto switch_id : beyond_tor) {
            switches.insert(switch_id);
        }
        if (workers.size() == 1) {
            emit(jpSignal, 1); // single machine
        } else if (workers.size() > 1 && switches.size() == 1) { // distributed
            emit(jpSignal, 2);
        } else { // workers.size() > 1 && switches.size() > 1 {// multi-racks
            emit(jpSignal, 3);
        }
    }

    job->setNum_workers_allocated(placement.size());
    job->setStart_time(simTime());
    emit(jstSignal, simTime());
    emit(jwtSignal, simTime() - job->getSubmit_time());
    unsigned rank = 0;
    hierarchy->setup_job(job, placement);
    for (auto pair : placement) {
        auto wid = pair.first;
        auto gpus = pair.second;
        EV_DEBUG << "Use " << gpus << " GPUs of Worker " << wid << endl;
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
        EV_DEBUG << "Received submitted job " << job->getJob_id() << " at "
                        << simTime() << endl;
        jobs[job->getJob_id()] = job; // saved as a local copy, don't delete
        job->setKind(0); // use kind as number of workers that finished the job
        emit(jsmtSignal, job->getSubmit_time());
        while (tryDispatchAJob()) {
            // send jobs until nothing left or nothing can be placed
        }
    } else { // a worker reports a finished job
        auto local_copy = jobs[job->getJob_id()];
        auto num_received = local_copy->getKind() + 1;
        local_copy->setKind(num_received);
        // worker sets kind using its id
        free_gpus[job->getWorker_id()] += job->getGpu();
        if (uint32_t(local_copy->getKind())
                == local_copy->getNum_workers_allocated()) {
            // all workers finished
            EV_DEBUG << "Finished job " << job->getJob_id() << " at "
                            << simTime() << endl;
            emit(jctSignal, simTime());
            delete local_copy;
            jobs.erase(job->getJob_id());
            //TODO: clean up job related resources
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
