#ifndef JOBSCHEDULING_H_
#define JOBSCHEDULING_H_

class Job;

class JobScheduling {
public:
    virtual Job* pick_a_job_to_execute(const std::map<uint64_t, Job*>&) = 0;
    virtual ~JobScheduling() = default;
};

class Fifo: public JobScheduling {
public:
    Job* pick_a_job_to_execute(const std::map<uint64_t, Job*> &jobs) override {
        for (auto &pair: jobs) {
            if (pair.second->getStart_time() < 0) {
                return pair.second;
            }
        }
        return nullptr;
    }
};

#endif /* JOBSCHEDULING_H_ */
