#include "SwitchML_m.h"
using namespace omnetpp;

#define STACKSIZE    16384

class Allreducer: public cSimpleModule {
public:
    Allreducer() :
            cSimpleModule(STACKSIZE) {
    }
    //    virtual void activity() override;
protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;

private:
    bool busy { false };
    void do_one_allreduce();
    cQueue queue;
    cGate *serverOutGate;
    uint64_t num_slots;
    uint64_t num_updates;
};

Define_Module(Allreducer);

int f(cObject *a, cObject *b) {
    return 0;
}

void Allreducer::initialize() {
    queue = std::move(cQueue("queue", f));
    serverOutGate = getParentModule()->gate("port$o");
    num_slots = getParentModule()->par("NUM_SLOTS");
    num_updates = getParentModule()->par("NUM_UPDATES");

//    scheduleAt(simTime(), new cMessage);
}

void Allreducer::do_one_allreduce() {
    auto m = check_and_cast<AllreduceRequest*>(queue.pop());
    for (uint64_t slot = 0; slot < num_slots; ++slot) {
        auto start = slot * num_updates;
        if (start >= m->getSize())
            break;
    }
    // send packets
    busy = true;
    delete m;
}

void Allreducer::handleMessage(cMessage *msg) {
    switch (msg->getKind()) {
    case 0:
        // enqueue
        queue.insert(msg);
        if (!busy) {
            do_one_allreduce();
        }
        break;
    case 1:
        // allreduce done
        if (!queue.isEmpty()) {
            do_one_allreduce();
        } else {
            // wait for next allreduce arrival
            busy = false;
        }
        // notify someone else?
        break;
    default:
        break;
    }
    delete msg;
}

//void Allreducer::activity() {
//    // parent should have the port to send out switchml packets
//    auto serverOutGate = getParentModule()->gate("port$o");
//
//    cQueue queue("queue", f);
//
//    // wait for first AllreduceRequest
//    queue.insert(check_and_cast<AllreduceRequest*>(receive()));
//
//    while (!queue.isEmpty()) {
//        auto msg = (AllreduceRequest*) queue.pop();
//        // allreduce...
//
//    }
//}

//    auto msg = check_and_cast<JobInfo*>(receive());
//    auto rank = msg->getRank();
//    auto id = msg->getId();
//    auto iters = msg->getIters();
//    auto num_layers = msg->getGrad_sizesArraySize();
//    bool distributed = msg->getNum_workers_allocated() > 1;
//    can_do_fp.resize(num_layers, true);
//    EV << "Start Job " << id << " as rank " << rank << endl;
//
//    for (unsigned iter = 0; iter < iters; ++iter) {
//        for (size_t i = 0; i < num_layers; ++i) {
//            while (!can_do_fp[i]) {
//                auto ack = check_and_cast<LayerAck*>(receive());
//                can_do_fp[ack->getLayer()] = true;
//            }
//            wait(msg->getFp_times(i));
//            can_do_fp[i] = false;
//        }
//
//        for (size_t i = num_layers; i > 0; --i) {
//            wait(msg->getBp_times(i-1));
//            if (distributed) {
//                allreduce(msg);
//            } else {
//
//            }
//        }
//        // wait for num_layers msgs from server
//
////        msg->getFp_times(0);
//
//    }

//    // respond to CONN_REQ by CONN_ACK
//    EV << "client is  addr=" << clientAddr << ", sending DYNA_CONN_ACK\n";
//    pk->setName("DYNA_CONN_ACK");
//    pk->setKind(DYNA_CONN_ACK);
//    pk->setSrcAddress(ownAddr);
//    pk->setDestAddress(clientAddr);
//    pk->setServerProcId(getId());
//    sendDirect(pk, serverOutGate);
//
//    // process data packets until DISC_REQ comes
//    for ( ; ; ) {
//        EV << "waiting for DATA(query) (or DYNA_DISC_REQ)\n";
//        pk = check_and_cast<DynaPacket *>(receive());
//        int type = pk->getKind();
//
//        if (type == DYNA_DISC_REQ)
//            break;
//
//        if (type != DYNA_DATA)
//            throw cRuntimeError("protocol error!");
//
//        datapk = (DynaDataPacket *)pk;
//
//        EV << "got DATA(query), processing...\n";
//        wait((double)processingTime);
//
//        EV << "sending DATA(result)\n";
//        datapk->setName("DATA(result)");
//        datapk->setKind(DYNA_DATA);
//        datapk->setSrcAddress(ownAddr);
//        datapk->setDestAddress(clientAddr);
//        datapk->setPayload("result");
//        sendDirect(datapk, serverOutGate);
//    }
//
//    // connection teardown in response to DISC_REQ
//    EV << "got DYNA_DISC_REQ, sending DYNA_DISC_ACK\n";
//    pk->setName("DYNA_DISC_ACK");
//    pk->setKind(DYNA_DISC_ACK);
//    pk->setSrcAddress(ownAddr);
//    pk->setDestAddress(clientAddr);
//    sendDirect(pk, serverOutGate);
//
//    EV << "exiting\n";
//    deleteModule();

