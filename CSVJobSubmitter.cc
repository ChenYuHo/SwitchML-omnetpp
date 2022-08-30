#include "SwitchML_m.h"
#define CSV_IO_NO_THREAD
#include "csv.h"
using namespace omnetpp;

class CSVJobSubmitter: public cSimpleModule {
private:
protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
};

Define_Module(CSVJobSubmitter);

void CSVJobSubmitter::initialize() {
    scheduleAt(simTime(), new cMessage);
}

void CSVJobSubmitter::handleMessage(cMessage *msg) {
    if (!msg->isSelfMessage()) {
        EV_WARN << "Shouldn't receive messages from outside, ignored" << endl;
        delete msg;
        return;
    }

    // start reading csv
    io::CSVReader<5/*column count*/> csvin(par("file").stringValue());
    csvin.read_header(io::ignore_missing_column, "num_gpu", "duration",
            "submit_time", "iterations", "model");
    unsigned num_gpu;
    unsigned duration;
    unsigned submit_time; // originally in seconds
    unsigned iterations = 0;
    std::string model = "resnet50";
//    unsigned counter = 0;
    unsigned max_jobs = par("max_jobs_to_submit");
    unsigned shrink_iter_factor = par("shrink_iter_factor");
    unsigned gpu_scale_factor = par("gpu_scale_factor");
    unsigned jid = 1;
    bool submit_all_when_start = par("submit_all_when_start");

    while (csvin.read_row(num_gpu, duration, submit_time, iterations, model)) {
        auto iters = iterations / shrink_iter_factor;
        if (iters == 0)
            iters = 1;

        auto job_info = new Job;
        job_info->setGpu(num_gpu*gpu_scale_factor);
        job_info->setIters(iters);
        job_info->setModel(model.c_str());
        job_info->setJob_id(jid++);
        sendDelayed((cMessage *) job_info, submit_all_when_start ? 0 : submit_time-simTime(), "out");
        if ((--max_jobs) == 0) {
            break;
    }
}
}

