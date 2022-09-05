#include "hierarchy.h"
#include "JobDispatcher.h"
#include "SwitchML_m.h"
#include <unordered_set>

void TwoLayers::process_hierarchy_query(HierarchyQuery* q) {
    core_switch = (Switch*) q->getModules(2);
    core_switch_id = q->getPath(2);
}

void TwoLayers::setup_job(Job *job, const std::unordered_map<int, unsigned>& placement) {
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
        for (auto wid: wids) {
            setup->appendIds(wid);
        }
        setup->setTop_level(top_level);
        job_dispatcher->sendDirect(setup, job_dispatcher->tors[tor_id], "directin");
        EV_DEBUG << fmt::format("ToR {} should receive {} num_updates (toplevel {}) for job {}\n",
                tor_id, num_updates, top_level, job_id);
        setup_for_core->appendIds(tor_id);
    }

    setup_for_core->setJob_id(job_id);
    setup_for_core->setTop_level(true);
    setup_for_core->setKind(6);
    job_dispatcher->sendDirect(setup_for_core, core_switch, "directin");
    EV_DEBUG << fmt::format("Core {} should receive {} num_updates (toplevel {}) for job {}\n",
                    core_switch_id, setup_for_core->getIdsArraySize(), true, job_id);
}
