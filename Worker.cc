#include "Worker.h"
#include "SwitchML_m.h"
#define FMT_HEADER_ONLY
#include "fmt/format.h"
using namespace omnetpp;
Define_Module(Worker);

void Worker::initialize() {
    id = getIndex();
    free_gpus = par("num_gpus");
    srvProcType = cModuleType::get("TrainingProcess");
    out_gate = gate("port$o");
    num_slots = par("num_slots");
    num_updates = par("num_updates");
}

int Worker::get_tor_id() {
    if (!tor)
        tor = (Switch*) gate("port$o")->getPathEndGate()->getOwnerModule();

    return tor->getId();
}

void Worker::start_job(Job *job) {
//    cModule *mod = srvProcType->createScheduleInit(fmt::format("{}test", ), this);
//            EV << "Start Server Process" << endl;
//            sendDirect(msg, mod, "in");

}

void Worker::sendNextPacket(SwitchMLPacket *pkt, uint32_t next_offset) {
    auto p = pkt->dup();
    p->setFrom_id(getId());
    p->setVer(1 - pkt->getVer());
    p->setOffset(next_offset);
    p->setUpward(true);
    send(p, out_gate);
    // caller should delete pkt
}

void Worker::handleMessage(cMessage *msg) {
    if (msg->isSelfMessage() || !msg->isPacket()) {
        EV << "Got Job Info" << endl;
        // mod will self destroy
        cModule *mod = srvProcType->createScheduleInit(
                fmt::format("{}i", msg->getId()).c_str(), this);
        EV << "Start Server Process" << endl;
        sendDirect(msg, mod, "in");
        auto job = check_and_cast<Job*>(msg);
        training_process_for_job[job->getJob_id()] = (TrainingProcess*) mod;
    } else {
        // SwitchMLPacket
        auto p = check_and_cast<SwitchMLPacket*>(msg);
//        myprintf(8, "[%lu] mid %d got aggregated pkt %s\n", eventlist().now(),
//                id, p->to_str().c_str());

        auto &set = received_pkts[p->getTensor_key()];
        if (set.find(p->getOffset()) != set.end()) {
            // duplicate
//            myprintf(8, "already received %d/%d pkt, discarding\n",
//                    p->offset / NUM_UPDATES, p->tensor->num_pkts_expected);
        } else {
            set.insert(p->getOffset());
            // cancel timer
            if (set.size() == p->getNum_pkts_expected()) {
                // allreduce done, notify training process
                auto training_process =
                        this->training_process_for_job[p->getJob_id()];
                auto ack = new LayerAck();
                ack->setKind(1);
                ack->setLayer(p->getLayer());
                training_process->forward_ack(ack);
                // can't clear yet if loss recovery is enabled, clear when job is done
                set.clear();
                received_pkts.erase(p->getTensor_key());
            } else {
                auto next_offset = p->getOffset() + num_slots * num_updates;
                if (next_offset < p->getGrad_size()) {
                    sendNextPacket(p, next_offset);
                }
            }
        }
        delete p;
    }
}

