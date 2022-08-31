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
        string file = "1.csv";
        bool submit_all_when_start = default(false);
    gates:
        output out;
}

simple JobDispatcher
{
    parameters:
        string scheduler = default("fifo");
        string placer = default("random");
    gates:
        input in;
        input directin;
}

moduleinterface ICollectiveScheduler
{
    gates:
        input in;
}

simple Sincronia like ICollectiveScheduler
{
    gates:
        input in;
}

network TwoLayers
{
    parameters:
        **.num_updates = 20;
        **.num_slots = 2;
        int switch_ports = default(3);
        int n_workers = switch_ports*(switch_ports-1);
        job_submitter.max_jobs_to_submit = 2;
        collective_scheduler.typename = "";
    submodules:
        core: Switch {
            gates:
                port_down[parent.switch_ports];
        }

        tors[switch_ports]: Switch {
            gates:
                port_up[1];
                port_down[parent.switch_ports-1];
        }
        workers[n_workers]: Worker;
        job_submitter: <default("CSVJobSubmitter")> like IJobSubmitter;
        job_dispatcher: JobDispatcher;
        collective_scheduler: <default("Sincronia")> like ICollectiveScheduler if typename!="";
    connections allowunconnected:
        // ToR to Core
        for i=0..switch_ports-1 {   
            tors[i].port_up[0] <--> Channel <--> core.port_down[i];
        }
        // Worker to ToR
        for i=0..n_workers-1 {
            workers[i].port <--> Channel <--> tors[int(i/(switch_ports-1))].port_down[i%(switch_ports-1)];
        }
        job_submitter.out --> job_dispatcher.in;
}
