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
    collective_scheduler = this->getSimulation()->findModuleByPath(
            "<root>.collective_scheduler");
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
        EV_DEBUG << "Worker " << getId() << " " << __LINE__ << endl;
        // mod will self destroy
        cModule *mod = srvProcType->createScheduleInit(
                fmt::format("{}i", msg->getId()).c_str(), this);
        sendDirect(msg, mod, "in");
        auto job = check_and_cast<Job*>(msg);
        training_process_for_job[job->getJob_id()] = (TrainingProcess*) mod;
        EV << "Worker "<< getId() << " Start Server Process for Job " << job->getJob_id() << endl;
    } else {
        EV_DEBUG << "Worker " << getId() << " " << __LINE__ << endl;
        // SwitchMLPacket
        auto p = check_and_cast<SwitchMLPacket*>(msg);
//        myprintf(8, "[%lu] mid %d got aggregated pkt %s\n", eventlist().now(),
//                id, p->to_str().c_str());

        auto &set = received_pkts[p->getTensor_key()];
        if (set.find(p->getOffset()) != set.end()) {
            EV_DEBUG << "Worker " << getId() << " " << __LINE__ << endl;
            // duplicate
//            myprintf(8, "already received %d/%d pkt, discarding\n",
//                    p->offset / NUM_UPDATES, p->tensor->num_pkts_expected);
        } else {
            EV_DEBUG << "Worker " << getId() << " " << __LINE__ << endl;
            set.insert(p->getOffset());
            // cancel timer
            if (set.size() == p->getNum_pkts_expected()) {
                EV_DEBUG << "Worker " << getId() << " " << __LINE__ << endl;
                // allreduce done, notify allreducer
                EV_DEBUG << fmt::format("Worker {} done allreduce\n", getId());
                auto training_process =
                        this->training_process_for_job[p->getJob_id()];
                auto allreducer = training_process->getSubmodule("allreducer");
                auto ack = new LayerAck();
                ack->setKind(1);
                ack->setLayer(p->getLayer());
                ack->setWeight_update_time(
                        training_process->get_weight_update_time(
                                p->getLayer()));
                ack->setCompleted(p->getChunk_id()+1 == p->getNum_chunks());
                if (collective_scheduler) {
                    EV_DEBUG << "Worker " << getId() << " " << __LINE__ << endl;
                    auto dup = ack->dup();
                    dup->setKind(2);
                    sendDirect(dup, collective_scheduler, "in");
                }
                sendDirect(ack, allreducer, "in");

                // can't clear yet if loss recovery is enabled
                set.clear();
                received_pkts.erase(p->getTensor_key());
            } else {
                EV_DEBUG << "Worker " << getId() << " " << __LINE__ << endl;
                auto next_offset = p->getOffset() + num_slots * num_updates;
                if (next_offset < p->getGrad_size()) {
                    sendNextPacket(p, next_offset);
                }
            }
        }
        delete p;
    }
}

