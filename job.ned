moduleinterface IJobSubmitter
{
    parameters:
//        int max_jobs_to_submit;
//        int shrink_iter_factor;
//        int gpu_scale_factor;
//        bool submit_all_when_start;
    gates:
        input directin;
        output jobout;
}

simple NJobSubmitter like IJobSubmitter
{
    parameters:
        int num_jobs = default(1);
        int num_gpus_per_job = default(8);
        int iters = default(1);
        string model = default("resnet50");
        double submit_interval @unit(us) = default(0us);
    gates:
        input directin;
        output jobout;
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
        input directin;
        output jobout;
}

simple JobDispatcher
{
    parameters:
        @signal[iterTime](type=simtime_t);
        @signal[jobCompletionTime](type=simtime_t);
        @signal[jctInflation](type=double);
        @signal[jobSubmissionTime](type=simtime_t);
        @signal[jobWaitTime](type=simtime_t);
        @signal[jobStartTime](type=simtime_t);
        @signal[jobPlacementType](type=int);
        string hierarchy = default("two_layers");
        string job_scheduling = default("fifo");
        string job_placement = default("random_multiracks");
        bool custom_model = default(false);
        string custom_model_sizes = default("");
        string custom_fp_times = default("");
        string custom_bp_times = default("");
        string custom_wu_times = default("");
        string custom_placement = default("");
        int custom_iters = default(0);
    gates:
        input jobin;
        input directin;
}
