#ifndef JOB_SCHEDULING_H_
#define JOB_SCHEDULING_H_

class Job;

class JobScheduling {
public:
    virtual Job* pick_a_job_to_execute(const std::map<uint64_t, Job*>&) = 0;
    virtual ~JobScheduling() = default;
};

class Fifo: public JobScheduling {
public:
    Job* pick_a_job_to_execute(const std::map<uint64_t, Job*> &jobs) override {
        if (jobs.empty())
            return nullptr;
        return jobs.begin()->second; // sorted by jid
    }
};

#endif /* JOB_SCHEDULING_H_ */
