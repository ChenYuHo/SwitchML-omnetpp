#include <cstdio>
#include <cstring>
#include <omnetpp.h>

using namespace omnetpp;

/**
 * Point-to-point interface module. While one frame is transmitted,
 * additional frames get queued up; see NED file for more info.
 */
class L2Queue: public cSimpleModule {
private:
//    intval_t frameCapacity;

    cQueue queue;
    cMessage *endTransmissionEvent = nullptr;
    cChannel *channel;
    bool isBusy;

//    simsignal_t qlenSignal;
//    simsignal_t busySignal;
//    simsignal_t queueingTimeSignal;
//    simsignal_t dropSignal;
//    simsignal_t txBytesSignal;
//    simsignal_t rxBytesSignal;

public:
    virtual ~L2Queue();

protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
//    virtual void refreshDisplay() const override;
    virtual void startTransmitting(cMessage *msg);
};

Define_Module(L2Queue);

L2Queue::~L2Queue() {
    cancelAndDelete(endTransmissionEvent);
}

void L2Queue::initialize() {
    queue.setName("queue");
    endTransmissionEvent = new cMessage("endTxEvent");
    channel = gate("outside$o")->findTransmissionChannel();

//    if (par("useCutThroughSwitching"))
//        gate("line$i")->setDeliverImmediately(true);

//    frameCapacity = par("frameCapacity");

//    qlenSignal = registerSignal("qlen");
//    busySignal = registerSignal("busy");
//    queueingTimeSignal = registerSignal("queueingTime");
//    dropSignal = registerSignal("drop");
//    txBytesSignal = registerSignal("txBytes");
//    rxBytesSignal = registerSignal("rxBytes");

//    emit(qlenSignal, queue.getLength());
//    emit(busySignal, false);
    isBusy = false;
}

void L2Queue::startTransmitting(cMessage *msg) {
//    EV << "Starting transmission of " << msg << endl;
    isBusy = true;
//    int64_t numBytes = check_and_cast<cPacket *>(msg)->getByteLength();
    send(msg, "outside$o");

//    emit(txBytesSignal, numBytes);

// Schedule an event for the time when last bit will leave the gate.
    simtime_t endTransmission = channel ? channel->getTransmissionFinishTime() : simTime();
    scheduleAt(endTransmission, endTransmissionEvent);
}

void L2Queue::handleMessage(cMessage *msg) {
    if (msg == endTransmissionEvent) {
        // Transmission finished, we can start next one.
//        EV << "Transmission finished.\n";
        isBusy = false;
        if (!queue.isEmpty()) {
//            emit(busySignal, false);
//        }
//        else {
            msg = (cMessage*) queue.pop();
//            emit(queueingTimeSignal, simTime() - msg->getTimestamp());
//            emit(qlenSignal, queue.getLength());
            startTransmitting(msg);
        }
    } else if (msg->arrivedOn("outside$i")) {
        // pass up
//        emit(rxBytesSignal, (intval_t)check_and_cast<cPacket *>(msg)->getByteLength());
        send(msg, "inside$o");
    } else if (msg->getKind() == 4) { // Worker queue get from Worker, send to Switch queue
        msg->setKind(5);
        sendDirect(msg, gate("outside$o")->getPathEndGate()->getOwnerModule(), "directin");
    } else if (msg->getKind() == 5) { // Switch queue get from Worker queue, send to Switch
        msg->setKind(4);
        sendDirect(msg, gate("inside$o")->getPathEndGate()->getOwnerModule(), "directin");
    } else {  // arrived on gate "inside$i"
        if (endTransmissionEvent->isScheduled()) {
            // We are currently busy, so just queue up the packet.
//            if (frameCapacity && queue.getLength() >= frameCapacity) {
//                EV << "Received " << msg << " but transmitter busy and queue full: discarding\n";
//                emit(dropSignal, (intval_t)check_and_cast<cPacket *>(msg)->getByteLength());
//                delete msg;
//            }
//            else {
//                EV << "Received " << msg << " but transmitter busy: queueing up\n";
            msg->setTimestamp();
            queue.insert(msg);
//                emit(qlenSignal, queue.getLength());
//            }
        } else {
            // We are idle, so we can start transmitting right away.
//            EV << "Received " << msg << endl;
//            emit(queueingTimeSignal, SIMTIME_ZERO);
            startTransmitting(msg);
//            emit(busySignal, true);
        }
    }
}

//void L2Queue::refreshDisplay() const
//{
//    getDisplayString().setTagArg("t", 0, isBusy ? "transmitting" : "idle");
//    getDisplayString().setTagArg("i", 1, isBusy ? (queue.getLength() >= 3 ? "red" : "yellow") : "");
//}
