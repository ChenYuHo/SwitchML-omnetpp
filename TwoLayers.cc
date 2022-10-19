#include "JobDispatcher.h"
#include "SwitchML_m.h"
#include <unordered_set>

#include "Hierarchy.h"
#define FMT_HEADER_ONLY
#include "fmt/format.h"

TwoLayers::TwoLayers(JobDispatcher *job_dispatcher) :
        job_dispatcher(job_dispatcher) {
    core_switch_id = job_dispatcher->getParentModule()->findSubmodule("core");
    core_switch = job_dispatcher->getSimulation()->getModule(core_switch_id);
}

std::unordered_set<int> TwoLayers::switch_ids_beyond_tors(
        std::unordered_set<int> tor_ids) {
    return tor_ids.size() > 1 ?
            std::unordered_set<int> { core_switch_id } :
            std::unordered_set<int> { };
}

void TwoLayers::setup_job(Job *job,
        const std::unordered_map<int, unsigned> &placement) {
    auto job_id = job->getJob_id();
    std::unordered_map<int, std::unordered_set<int>> wids_for_tor { };
    for (auto &pair : placement) {
        auto wid = pair.first;
        wids_for_tor[job_dispatcher->tor_id_for_worker[wid]].insert(wid);
    }

    auto setup_for_core = new Setup;

    for (auto &pair : wids_for_tor) {
        auto tor_id = pair.first;
        auto &wids = pair.second;
        auto setup = new Setup;
        auto num_updates = wids.size();
        auto top_level = job->getNum_workers_allocated() == num_updates;
        setup->setJob_id(job_id);
        setup->setKind(6);
        for (auto wid : wids) {
            setup->appendIds(wid);
        }
        setup->setTop_level(top_level);
        job_dispatcher->sendDirect(setup,
                job_dispatcher->getSimulation()->getModule(tor_id), "directin");
        EV_DEBUG << "[TwoLayers]\t" << simTime()
                        << fmt::format(
                                "\tToR {} should receive {} num_updates (toplevel {}) for job {}\n",
                                tor_id, num_updates, top_level, job_id);
        setup_for_core->appendIds(tor_id);
    }

    if (wids_for_tor.size() == 1) {
        // no need to involve core switch
        delete setup_for_core;
        return;
    }

    setup_for_core->setJob_id(job_id);
    setup_for_core->setTop_level(true);
    setup_for_core->setKind(6);
    job_dispatcher->sendDirect(setup_for_core, core_switch, "directin");
    EV_DEBUG << "[TwoLayers]\t" << simTime()
                    << fmt::format(
                            "\tCore {} should receive {} num_updates (toplevel {}) for job {}\n",
                            core_switch_id, setup_for_core->getIdsArraySize(),
                            true, job_id);
}
