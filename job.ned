moduleinterface IJobSubmitter
{
    parameters:
        int max_jobs_to_submit;
        int shrink_iter_factor;
        int gpu_scale_factor;
        bool submit_all_when_start;
    gates:
        inout port;
}

simple CSVJobSubmitter like IJobSubmitter
{
    parameters:
        int max_jobs_to_submit = default(0);
        int shrink_iter_factor = default(1);
        int gpu_scale_factor = default(1);
        string file = default("60_job.csv");
        bool submit_all_when_start = default(false);
    gates:
        inout port;
}

simple JobDispatcher
{
    parameters:
        @signal[jobCompletionTime](type=simtime_t);
        @statistic[jct](source=jobCompletionTime; record=histogram,vector);
        @signal[jobSubmissionTime](type=simtime_t);
        @statistic[jsmt](source=jobSubmissionTime; record=histogram,vector);
        @signal[jobWaitTime](type=simtime_t);
        @statistic[jwt](source=jobWaitTime; record=histogram,vector);
        @signal[jobStartTime](type=simtime_t);
        @statistic[jst](source=jobStartTime; record=histogram,vector);
        @signal[jobPlacementType](type=int);
        @statistic[jp](source=jobPlacementType; record=histogram,vector);
        string hierarchy = default("two_layers");
        string job_scheduling = default("fifo");
        string job_placement = default("random_multirack");
    gates:
        inout port;
        inout worker_ports[];
        input directin;
}
