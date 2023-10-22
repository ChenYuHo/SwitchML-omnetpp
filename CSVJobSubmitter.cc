#include "SwitchML_m.h"
#include "ModelStats.h"
#define CSV_IO_NO_THREAD
#include "csv.h"
using namespace omnetpp;

class CSVJobSubmitter: public cSimpleModule {
private:
    void initialize() override;
    void handleMessage(cMessage *msg) override;
};

Define_Module(CSVJobSubmitter);

void CSVJobSubmitter::initialize() {
}

void CSVJobSubmitter::handleMessage(cMessage *msg) {
    delete msg;

    // start reading csv
    io::CSVReader<5/*column count*/> csvin(par("file").stringValue());
    csvin.read_header(io::ignore_missing_column, "num_gpu", "duration",
            "submit_time", "iterations", "model");
    unsigned num_gpu;
    unsigned duration;
    unsigned submit_time; // originally in seconds
    unsigned iterations = 0;
    std::string m = "resnet50";
    unsigned max_jobs = par("max_jobs_to_submit");
    unsigned shrink_iter_factor = par("shrink_iter_factor");
    unsigned gpu_scale_factor = par("gpu_scale_factor");
    bool submit_all_when_start = par("submit_all_when_start");
    auto jobs = std::vector<Job*> { };
    while (csvin.read_row(num_gpu, duration, submit_time, iterations, m)) {
        auto iters = iterations / shrink_iter_factor;
        if (iters == 0)
            iters = 1;
        auto job_info = new Job;
        job_info->setGpu(num_gpu * gpu_scale_factor);
        job_info->setIters(iters);
        job_info->setSubmit_time(SimTime(submit_time, SIMTIME_S));
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
        jobs.push_back(job_info);
        if ((--max_jobs) == 0) {
            break;
        }
    }

    std::sort(jobs.begin(), jobs.end(),
            [](Job *a, Job *b) {
                return a->getSubmit_time() == b->getSubmit_time() ?
                        a->getJob_id() < b->getJob_id() :
                        a->getSubmit_time() < b->getSubmit_time();
            });

    // re-id jobs, smaller jid has earlier submission time
    uint64_t jid = 0;
    for (auto job_info : jobs) {
        job_info->setJob_id(jid++);
	if (submit_all_when_start) send(job_info, "jobout");
	else sendDelayed(job_info, job_info->getSubmit_time(), "jobout");
    }
}

