#include "Switch.h"
#define FMT_HEADER_ONLY
#include "fmt/format.h"

Define_Module(Switch);


void Switch::initialize() {
//    id = getIndex();
//    free_gpus = par("num_gpus");
//    srvProcType = cModuleType::get("TrainingProcess");
}

void Switch::multicast_downward(SwitchMLPacket *pkt) {
    for (int i=0; i<gateSize("port_down$o"); ++i) {
        pkt->setUpward(false);
        pkt->setFrom_id(getId());
        auto g = gate("port_down$o", i);
        this->send(pkt->dup(), g);
    }
}


void Switch::handleMessage(cMessage *msg) {
    auto p = check_and_cast<SwitchMLPacket*>(msg);
    auto key = fmt::format("s{}v{}", p->getSlot(), p->getVer());
    auto key_of_the_other_slot = fmt::format("s{}v{}", p->getSlot(), 1 - p->getVer());
    if (p->getUpward()) {
        auto &seen = seen_for_tensor_key[p->getTensor_key()];
        auto &count = count_for_tensor_key[p->getTensor_key()];
        auto &seen_key = seen[key];
        if (seen_key.find(p->getFrom_id()) != seen_key.end()) { // shadow buffer
//            if (count[key] == p->n_workers) {
//                auto dest = p->id;
//                assert(down_routes.contains(dest));
//                Route *route = down_routes[dest];
//                auto unicast_pkt = copy_pkt(p, route, false);
//                myprintf(11, "[%lu] Switch layer %d id %d reply shadow buffer packet downward to wid/sid %d %s\n",
//                         eventlist().now(), layer, id, dest, unicast_pkt->to_str().c_str());
//                unicast_pkt->sendOnSimple();
//            } else if (top_level_for_job[p->job_id] && count[key] == 0) {
//                auto unicast_pkt = copy_pkt(p, up_route, true);
//                myprintf(11, "[%lu] Switch layer %d id %d forward shadow buffer request packet upward %s\n",
//                         eventlist().now(), layer, id, unicast_pkt->to_str().c_str());
//                unicast_pkt->sendOnSimple();
//            } // else drop (free) packet
        } else {
            seen_key.insert(p->getFrom_id());
            seen[key_of_the_other_slot].erase(p->getFrom_id());
            count[key] = ((count[key] + 1) % p->getN_workers()) % num_updates_for_job[p->getJob_id()];
            if (count[key] == 0) {
                // done aggregation
                if (top_level_for_job[p->getJob_id()]) { // downward
                    count[key] = p->getN_workers();
                    multicast_downward(p);
                } else {  // upward
                    // send to upper level
                    auto pkt = p->dup();
                    pkt->setFrom_id(getId());
                    this->send(pkt, gate("port_up$o", 0));
                }
            } // else drop (free) packet
        }
    } else {
        auto &count = count_for_tensor_key[p->getTensor_key()];
//        // received from upper level switch
        count[key] = p->getN_workers();
        multicast_downward(p);
    }
    delete p;
}
