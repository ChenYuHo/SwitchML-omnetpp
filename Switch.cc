#include "Switch.h"
#define FMT_HEADER_ONLY
#include "fmt/format.h"

Define_Module(Switch);

void Switch::initialize() {
    for (int i = 0; i < gateSize("up_ports"); ++i) {
        auto g = gate("up_ports$o", i);
        auto gid = g->getId();
        // use cPacket because gid is often larger than cMessage kind (short) can accommodate
        endTransmissionEvents[gid] = new cPacket("endTxEvent", 0, gid);
        channels[gid] = g->findTransmissionChannel();
        upper_level_switch = (Switch*) g->getPathEndGate()->getOwnerModule();
    }
    for (int i = 0; i < gateSize("down_ports"); ++i) {
        auto g = gate("down_ports$o", i);
        auto gid = g->getId();
        auto remote_id = g->getPathEndGate()->getOwnerModule()->getId();
        gate_id[remote_id] = gid;
        endTransmissionEvents[gid] = new cPacket("endTxEvent", 0, gid);
        channels[gid] = g->findTransmissionChannel();
    }
}

void Switch::clean_resources_for_tensor(uint64_t tensor_key) {
    count_for_tensor_key.erase(tensor_key);
    seen_for_tensor_key.erase(tensor_key);
}

void Switch::clean_resources_for_job(uint64_t jid) {
    num_updates_for_job.erase(jid);
    top_level_for_job.erase(jid);
    gate_ids_for_job.erase(jid);
}

void Switch::try_send(cPacket *pkt, int gid) {
    if (endTransmissionEvents[gid]->isScheduled()) {
        // We are currently busy, so just queue up the packet.
        pkt->setTimestamp();
        queues[gid].insert(pkt);
//        EV_DEBUG << "queued pkt slot " << ((SwitchMLPacket*) pkt)->getSlot()
//                        << endl;
    } else {
        // We are idle, so we can start transmitting right away.
        startTransmitting(pkt, gid);
//        EV_DEBUG << "start transmitting pkt slot "
//                        << ((SwitchMLPacket*) pkt)->getSlot() << endl;
    }
}

void Switch::startTransmitting(cMessage *msg, int gid) {
    port_isBusy[gid] = true;
    send(msg, gid);
    auto channel = channels[gid];
    simtime_t endTransmission =
            channel ? channel->getTransmissionFinishTime() : simTime();
    scheduleAt(endTransmission, endTransmissionEvents[gid]);
    EV_DEBUG << "endTransmission scheduled at " << endTransmission << endl;
}

void Switch::multicast_downward(SwitchMLPacket *pkt) {
    EV_DEBUG << "Switch " << getId() << " multicast downward pkt job "
                    << pkt->getJob_id() << " gate ids: ";
    for (auto gid : gate_ids_for_job[pkt->getJob_id()]) {
        EV_DEBUG << gid << " ";
        pkt->setUpward(false);
        pkt->setFrom_id(getId());
        try_send(pkt->dup(), gid);
    }
    EV_DEBUG << endl;
}

void Switch::handleMessage(cMessage *msg) {
    if (msg->isSelfMessage()) {
        auto pkt = (cPacket*) msg;
        // Transmission finished, we can start next one.
        EV_DEBUG << "Transmission of gate " << pkt->getBitLength()
                        << " finished at " << simTime() << endl;
        auto gid = int(pkt->getBitLength());
        auto &queue = queues[gid];
        port_isBusy[gid] = false;
        if (!queue.isEmpty()) {
            EV_DEBUG << "Start next transmission of gate " << gid << endl;
            startTransmitting((cMessage*) queue.pop(), gid);
        }
        return;
    }
    if (!msg->isPacket()) {
        switch (msg->getKind()) {
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
    EV_DEBUG << "Switch " << getId() << " get packet slot " << p->getSlot()
                    << " at " << simTime() << endl;
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
                EV_DEBUG
                                << fmt::format(
                                        "Switch {} done slot {} offset {}\n",
                                        getId(), p->getSlot(), p->getOffset());
                // done aggregation for this slot
                if (top_level_for_job[p->getJob_id()]) { // downward
                    count[key] = p->getN_workers();
                    multicast_downward(p);
                } else {  // upward
                    // send to upper level
                    auto pkt = p->dup();
                    pkt->setFrom_id(getId());
                    try_send(pkt, gate("up_ports$o", 0)->getId());
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

Switch::~Switch() {
    for (auto &pair : endTransmissionEvents) {
        cancelAndDelete(pair.second);
    }
}
