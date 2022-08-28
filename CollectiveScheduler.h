#ifndef COLLECTIVESCHEDULER_H_
#define COLLECTIVESCHEDULER_H_
#include "SwitchML_m.h"

class CollectiveScheduler: public omnetpp::cSimpleModule {
public:
    virtual void enqueue() = 0;
};

#endif /* COLLECTIVESCHEDULER_H_ */
