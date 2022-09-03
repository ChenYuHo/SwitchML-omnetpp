#ifndef SINCRONIA_H_
#define SINCRONIA_H_

#include "SwitchML_m.h"
#include <unordered_map>
using namespace omnetpp;

class Sincronia: public cSimpleModule {
private:
    std::unordered_map<uint64_t, std::vector<CollectiveOperationRequest*>> queue { };
protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
};

#endif /* SINCRONIA_H_ */
