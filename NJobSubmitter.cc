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
    std::string model = par("model");
    double submit_interval = par("submit_interval");
    auto interval = SimTime(submit_interval, SIMTIME_US);
    EV_DEBUG << "submit_interval " << submit_interval << " interval " << interval << endl;
    auto submit_time = SimTime::ZERO;
    uint64_t jid = 1;
    std::vector<std::string> models;
    std::istringstream iss(model);
    std::string model_str;
    while (std::getline(iss, model_str, ',')) {
        models.push_back(model_str);
    }
    int num_jobs = par("num_jobs");
    int min_size = std::min(num_jobs, int(models.size()));

    for (int i = 0; i < min_size; ++i) {
        auto job_info = new Job;
        job_info->setGpu(num_gpus);
        job_info->setIters(iters);
        job_info->setSubmit_time(submit_time);
        submit_time += interval;
        job_info->setJob_id(jid++);
        auto m = models[i];
        EV_DEBUG << "submit job " << i << " model " << m << " at "
                        << submit_time << endl;
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

