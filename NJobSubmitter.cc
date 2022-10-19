#include "SwitchML_m.h"
#include "ModelStats.h"
using namespace omnetpp;

class NJobSubmitter: public cSimpleModule {
private:
    void initialize() override;
    void handleMessage(cMessage *msg) override;
};

Define_Module(NJobSubmitter);

void NJobSubmitter::initialize() {
}

void NJobSubmitter::handleMessage(cMessage *msg) {
    delete msg;
    uint64_t num_gpus = par("num_gpus_per_job");
    uint64_t iters = par("iters");
    std::string m = par("model");
    uint64_t jid = 1;
    for (int i = 0; i < par("num_jobs").intValue(); ++i) {
        auto job_info = new Job;
        job_info->setGpu(num_gpus);
        job_info->setIters(iters);
        job_info->setSubmit_time(SimTime::ZERO);
        job_info->setJob_id(jid++);
        if (m == "alexnet") {
            job_info->setModel(alexnet);
        } else if (m == "vgg11") {
            job_info->setModel(vgg11);
        } else if (m == "vgg16") {
            job_info->setModel(vgg16);
        } else if (m == "vgg19") {
            job_info->setModel(vgg19);
        } else if (m == "inception" || m == "inception3" || m == "inception4") { // trace is inceptionv3
            job_info->setModel(inception);
        } else if (m == "googlenet") {
            job_info->setModel(googlenet);
        } else if (m == "resnet101") {
            job_info->setModel(resnet101);
        } else if (m == "resnet152") {
            job_info->setModel(resnet152);
        } else if (m == "bert") {
            job_info->setModel(bert);
        } else { // resnet50
            job_info->setModel(resnet50);
        }
        sendDelayed(job_info, job_info->getSubmit_time(), "jobout"); // to job dispatcher
    }
}

