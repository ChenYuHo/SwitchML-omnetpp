moduleinterface IJobSubmitter
{
    parameters:
        int max_jobs_to_submit;
        int shrink_iter_factor;
        int gpu_scale_factor;
        bool submit_all_when_start;
    gates:
        output out;
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
        output out;
}

simple JobDispatcher
{
    parameters:
        @signal[jobCompletionTime](type=simtime_t);
        string scheduler = default("fifo");
        string placer = default("random");
    gates:
        input in;
        input directin;
}