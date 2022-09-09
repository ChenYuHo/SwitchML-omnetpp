#include "Switch.h"
#define FMT_HEADER_ONLY
#include "fmt/format.h"
#include "Worker.h"

Define_Module(Switch);

void Switch::initialize() {
    for (int i = 0; i < gateSize("down_ports"); ++i) {
        auto g = gate("down_ports$o", i);
        auto id =
                g->getPathEndGate()->getOwnerModule()->gate("outside$o")->getPathEndGate()->getOwnerModule()->gate(
                        "inside$o")->getPathEndGate()->getOwnerModule()->getId();
        gate_id[id] = g->getId();
    }
}

void Switch::multicast_downward(SwitchMLPacket *pkt) {
    EV_DEBUG << "Switch " << getId() << " multicast downward" << endl;
    for (auto gate_id : gate_ids_for_job[pkt->getJob_id()]) {
        pkt->setUpward(false);
        pkt->setFrom_id(getId());
        send(pkt->dup(), gate_id);
    }
}

void Switch::handleMessage(cMessage *msg) {
    if (!msg->isPacket()) {
        switch (msg->getKind()) {
        case 4: {
            // HierarchyQuery
            auto q = (HierarchyQuery*) msg;
            q->appendPath(getId());
            q->appendModules(this);
            auto num_up_ports = gateSize("up_ports");
            if (num_up_ports) {
                for (int i = 0; i < num_up_ports; ++i) {
                    sendDirect(q->dup(),
                            gate("up_ports$o", i)->getPathEndGate()->getOwnerModule(),
                            "directin");
                }
                delete q;
            } else {
                // no up ports, send back to JobDispatcher
                this->sendDirect(q,
                        this->getSimulation()->getModule(q->getFrom_id()),
                        "directin");
            }
            break;
        }
        case 6: {
            // setup messages
            auto setup = (Setup*) msg;
            auto job_id = setup->getJob_id();
            auto num_updates = setup->getIdsArraySize();
            num_updates_for_job[job_id] = num_updates;
            top_level_for_job[job_id] = setup->getTop_level();
            for (size_t i = 0; i < num_updates; ++i) {
                gate_ids_for_job[job_id].insert(gate_id[setup->getIds(i)]);
            }
            delete msg;
            break;
        }
        default:
            EV_DEBUG
                            << fmt::format("wrong message of kind from {}\n",
                                    msg->getKind(),
                                    msg->getSenderModule()->getName());
            delete msg;
            EV_FATAL << "wrong message\n";
            break;
        }
        return;
    }
    auto p = (SwitchMLPacket*) msg;
    auto key = fmt::format("s{}v{}", p->getSlot(), p->getVer());
    auto key_of_the_other_slot = fmt::format("s{}v{}", p->getSlot(),
            1 - p->getVer());
    EV_DEBUG << "Switch " << getId() << " get packet" << endl;
    if (p->getUpward()) {
        auto &seen = seen_for_tensor_key[p->getTensor_key()];
        auto &count = count_for_tensor_key[p->getTensor_key()];
        auto &seen_key = seen[key];
        if (seen_key.find(p->getFrom_id()) != seen_key.end()) { // shadow buffer
            EV_FATAL << "Switch " << getId()
                            << " received shadow buffer request" << endl;
        } else {
            seen_key.insert(p->getFrom_id());
            seen[key_of_the_other_slot].erase(p->getFrom_id());
            count[key] = ((count[key] + 1) % p->getN_workers())
                    % num_updates_for_job[p->getJob_id()];
            if (count[key] == 0) {
                EV_DEBUG << "Switch " << getId() << " done allreduce" << endl;
                // done aggregation
                if (top_level_for_job[p->getJob_id()]) { // downward
                    count[key] = p->getN_workers();
                    multicast_downward(p);
                } else {  // upward
                    // send to upper level
                    auto pkt = p->dup();
                    pkt->setFrom_id(getId());
                    this->send(pkt, gate("up_ports$o", 0));
                }
            } // else drop (free) packet
        }
    } else { // received from upper level switch
        auto &count = count_for_tensor_key[p->getTensor_key()];
        count[key] = p->getN_workers();
        multicast_downward(p);
    }
    delete p;
}
