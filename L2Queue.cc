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
public:
    virtual ~L2Queue();

protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
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
//    frameCapacity = par("frameCapacity");
    isBusy = false;
}

void L2Queue::startTransmitting(cMessage *msg) {
    isBusy = true;
//    int64_t numBytes = check_and_cast<cPacket *>(msg)->getByteLength();
    send(msg, "outside$o");

    // Schedule an event for the time when last bit will leave the gate.
    simtime_t endTransmission =
            channel ? channel->getTransmissionFinishTime() : simTime();
    scheduleAt(endTransmission, endTransmissionEvent);
}

void L2Queue::handleMessage(cMessage *msg) {
    if (msg == endTransmissionEvent) {
        // Transmission finished, we can start next one.
        isBusy = false;
        if (!queue.isEmpty()) {
            msg = (cMessage*) queue.pop();
            startTransmitting(msg);
        }
    } else if (msg->arrivedOn("outside$i")) {
        // pass up
        send(msg, "inside$o");
    } else if (msg->getKind() == 4) { // Worker queue get from Worker, send to Switch queue
        msg->setKind(5);
        sendDirect(msg, gate("outside$o")->getPathEndGate()->getOwnerModule(),
                "directin");
    } else if (msg->getKind() == 5) { // Switch queue get from Worker queue, send to Switch
        msg->setKind(4);
        sendDirect(msg, gate("inside$o")->getPathEndGate()->getOwnerModule(),
                "directin");
    } else {  // arrived on gate "inside$i"
        if (endTransmissionEvent->isScheduled()) {
            // We are currently busy, so just queue up the packet.
//            if (frameCapacity && queue.getLength() >= frameCapacity) {
//                delete msg;
//            }
//            else {
            msg->setTimestamp();
            queue.insert(msg);
//            }
        } else {
            // We are idle, so we can start transmitting right away.
            startTransmitting(msg);
        }
    }
}
