#include "SwitchML_m.h"
#include "JobDispatcher.h"
#define FMT_HEADER_ONLY
#include "fmt/format.h"
#include "Hierarchy.h"
#include "JobScheduling.h"
#include "JobPlacement.h"
#include "Switch.h"
#include "ModelStats.h"

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
        jctInflation = registerSignal("jctInflation");
        jobStartTime = registerSignal("jobStartTime");
        jobWaitTime = registerSignal("jobWaitTime");
        jobPlacementType = registerSignal("jobPlacementType");
        bandwidth =
                int(
                        getModuleByPath("^.workers[0]")->gate("port$o")->getChannel()->par(
                                "datarate").doubleValue() / 1e9);
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
            EV_DEBUG << "Finished job " << job->getJob_id() << " in "
                            << simTime() - job->getStart_time()
                            << " seconds at " << simTime() << endl;
            auto jct = simTime() - job->getStart_time();
            emit(jobCompletionTime, jct);
            const static std::unordered_map<int,
                    std::unordered_map<short, std::vector<double>>> stats { {
                    10, { { alexnet, { 0, 0.278793241348, 0.55737556696,
                            0.83469933826, 1.11475938086, 1.395157222529,
                            1.670398885594, 1.950908785259, 2.224619898205,
                            2.506831098765, 2.783059617631 } }, { bert, { 0,
                            1.108804001754, 2.217549117911, 3.326187663706,
                            4.435087336378, 5.543779181392, 6.65224089474,
                            7.760963982902, 8.869883284171, 9.978401640874,
                            11.087420221838 } }, { googlenet, { 0,
                            0.140752798006, 0.280481296029, 0.420707135202,
                            0.562292494896, 0.697033570385, 0.838331232424,
                            0.977254132324, 1.12330872246, 1.267425673279,
                            1.39971256489 } }, { inception, { 0, 0.261207735776,
                            0.514299519726, 0.768556773223, 1.026880069447,
                            1.289616033231, 1.537818092938, 1.801634989431,
                            2.057853395381, 2.30384298898, 2.570719957517 } }, {
                            resnet101, { 0, 0.193342706147, 0.38504875631,
                                    0.578239414322, 0.770428136736,
                                    0.96370890181, 1.157690747172,
                                    1.348882671566, 1.540631853734,
                                    1.732838739041, 1.925654892063 } }, {
                            resnet152, { 0, 0.260553660237, 0.522149623781,
                                    0.782095674229, 1.042389970945,
                                    1.305042609276, 1.56415980096,
                                    1.823959206912, 2.087163080377,
                                    2.348446633418, 2.609033807548 } }, {
                            resnet50, { 0, 0.150167997092, 0.300700530453,
                                    0.45011673194, 0.602614385923,
                                    0.757969951707, 0.910110178978,
                                    1.055331510312, 1.208277785687,
                                    1.3571418023, 1.517912289484 } }, { vgg11, {
                            0, 0.466930762775, 0.934005549012, 1.40075226781,
                            1.867381407326, 2.336072643945, 2.801301827387,
                            3.269948321458, 3.735486430045, 4.203378670679,
                            4.66972228259 } }, { vgg16, { 0, 0.517781309667,
                            1.033360175055, 1.549026890472, 2.065870763964,
                            2.583554762271, 3.099346137239, 3.61830077596,
                            4.132520627097, 4.646790952865, 5.165279283306 } },
                            { vgg19, { 0, 0.546684114939, 1.091860588859,
                                    1.640966974968, 2.188246719992,
                                    2.733204753778, 3.279162297118,
                                    3.830324727136, 4.373830865702,
                                    4.924179140375, 5.471998435476 } } } }, {
                    100, { { alexnet, { 0, 0.138387987434, 0.276321785314,
                            0.413612672199, 0.554477611276, 0.694796962147,
                            0.832558935996, 0.971116481763, 1.110656451494,
                            1.245595607024, 1.384723404524 } }, { bert, { 0,
                            0.143448931791, 0.287209118384, 0.430575804095,
                            0.57693490138, 0.716009901951, 0.860486721777,
                            0.998156258897, 1.147253031982, 1.293546472667,
                            1.432855011156 } }, { googlenet, { 0,
                            0.139227817584, 0.278520560562, 0.420625581272,
                            0.560505408695, 0.696553426318, 0.839419994596,
                            0.976461858672, 1.110111366859, 1.250595777652,
                            1.388370363653 } }, { inception, { 0,
                            0.258299949637, 0.50981328387, 0.76580633752,
                            1.022762367513, 1.276915569241, 1.536962447231,
                            1.793830622519, 2.054432092287, 2.308295191755,
                            2.567478811083 } }, { resnet101, { 0, 0.13964541404,
                            0.280142716222, 0.420610712289, 0.561928450832,
                            0.702512862175, 0.8376423879, 0.978061502398,
                            1.120908340898, 1.260554771344, 1.396771002384 } },
                            { resnet152, { 0, 0.195501801144, 0.392171038343,
                                    0.584736557146, 0.782411947015,
                                    0.97579904618, 1.168724935033,
                                    1.367362112838, 1.560753687431,
                                    1.754439021996, 1.954446677614 } }, {
                                    resnet50, { 0, 0.149248521734,
                                            0.297182923917, 0.451250142388,
                                            0.599611654746, 0.749971071605,
                                            0.90476630969, 1.053424422633,
                                            1.201438378134, 1.352508563481,
                                            1.503244190793 } }, { vgg11, { 0,
                                    0.094933441665, 0.189783763658,
                                    0.284618862057, 0.38268250875,
                                    0.478901232129, 0.572248421525,
                                    0.667190589037, 0.768086513751,
                                    0.856477590156, 0.950461158257 } }, { vgg16,
                                    { 0, 0.204498354531, 0.417676835099,
                                            0.615681245397, 0.813248533891,
                                            1.028731825014, 1.224947582441,
                                            1.438608617745, 1.640701597503,
                                            1.850452174574, 2.050455090994 } },
                            { vgg19, { 0, 0.245712574877, 0.494826808212,
                                    0.743876718732, 0.993594371832,
                                    1.242322920731, 1.490827831116,
                                    1.740384326802, 1.98544017313,
                                    2.234498498207, 2.497596511112 } } } } };

            emit(jctInflation,
                    jct.dbl()
                            / stats.at(bandwidth).at(local_copy->getModel()).at(
                                    local_copy->getIters()));
            EV_DEBUG << jct.dbl() << " "
                            << stats.at(bandwidth).at(local_copy->getModel()).at(
                                    local_copy->getIters()) << " "
                            << jct.dbl()
                                    / stats.at(bandwidth).at(
                                            local_copy->getModel()).at(
                                            local_copy->getIters()) << endl;

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
