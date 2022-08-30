#ifndef SINCRONIA_H_
#define SINCRONIA_H_

#include "SwitchML_m.h"
using namespace omnetpp;

class Sincronia: public cSimpleModule {
private:
protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
};

#endif /* SINCRONIA_H_ */
