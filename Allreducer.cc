#include "SwitchML_m.h"
#include "Allreducer.h"
using namespace omnetpp;

Define_Module(Allreducer);

//int f(cObject *a, cObject *b) {
//    return 0;
//}

void Allreducer::initialize() {
    queue = cQueue("queue"); //, f);
    serverOutGate = getParentModule()->gate("port$o");
    num_slots = getParentModule()->par("num_slots");
    num_updates = getParentModule()->par("num_updates");

//    scheduleAt(simTime(), new cMessage);
}

void Allreducer::doOneAllreduce() {
    busy = true;
    auto m = check_and_cast<AllreduceRequest*>(queue.pop());
    auto grad_size = m->getSize();
    auto num_pkts_expected = grad_size / num_updates;
    if (grad_size % num_updates)
        num_pkts_expected += 1;
    for (uint64_t slot = 0; slot < num_slots; ++slot) {
        auto offset = slot * num_updates;
        if (offset >= grad_size)
            break;

        auto p = new SwitchMLPacket();
        p->setFrom_id(getId());
        p->setVer(0);
        p->setSlot(slot);
        p->setOffset(offset);
        p->setUpward(true);
        p->setLayer(m->getLayer());
        p->setTensor_key(m->getTensor_key());
        p->setN_workers(m->getNum_workers_allocated());
        p->setJob_id(m->getJob_id());
        p->setNum_pkts_expected(num_pkts_expected);
        p->setGrad_size(grad_size);
        send(p, serverOutGate);
    }
    delete m;
}

void Allreducer::handleMessage(cMessage *msg) {
    switch (msg->getKind()) {
    case 0:
        // enqueue
        queue.insert(msg);
        if (!busy) {
            doOneAllreduce();
        }
        break;
    case 1: {
        // LayerAck
        busy = false;
        auto ack = check_and_cast<LayerAck*>(msg);
        this->sendDelayed(ack, ack->getWeight_update_time(),
                getParentModule()->gate("in"));
        if (!queue.isEmpty()) {
            doOneAllreduce();
        }
        break;
    }
    default:
        delete msg;
        EV_FATAL << "got unexpected message" << endl;
        break;
    }
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

