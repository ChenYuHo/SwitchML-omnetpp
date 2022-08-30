#ifndef ALLREDUCER_H_
#define ALLREDUCER_H_
using namespace omnetpp;

class Allreducer: public cSimpleModule {
public:
    Allreducer() :
            cSimpleModule() {
    }
    //    virtual void activity() override;
protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;

private:
    bool busy { false };
    void doOneAllreduce();
    cQueue queue;
    cGate *serverOutGate;
    uint64_t num_slots;
    uint64_t num_updates;
};



#endif /* ALLREDUCER_H_ */
