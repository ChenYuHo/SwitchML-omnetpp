Define_Module(Sincronia);

void Sincronia::initialize() {
//    scheduleAt(simTime(), new cMessage);
}

void Sincronia::handleMessage(cMessage *msg) {
    auto request = check_and_cast<AllreduceRequest*>(msg);

}

