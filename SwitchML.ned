network Paper
{
    parameters:
        **.num_updates = default(256);
        **.num_slots = default(512);
        **.num_gpus = default(4);
        int switch_ports = default(8);
//        job_dispatcher.placer = "paper";
    submodules:
        tor: Switch {
            gates:
                port_down[parent.switch_ports];
        }
        workers[parent.switch_ports]: Worker {
            parameters:
                test_tensor_size = default(26214400);
        };
        job_submitter: <default("CSVJobSubmitter")> like IJobSubmitter;
        job_dispatcher: JobDispatcher;
    connections allowunconnected:
        // Worker to ToR
        for i=0..switch_ports-1 {
            workers[i].port <--> Channel <--> tor.port_down[i];
        }
        job_submitter.out --> job_dispatcher.in;
}

network TwoLayers
{
    parameters:
        **.num_updates = 256;
        **.num_slots = 512;
        int switch_ports = default(4);
        int n_workers = switch_ports*(switch_ports-1);
        job_submitter.max_jobs_to_submit = 2;
        collective_scheduler.typename = "";
        job_submitter.file = "60_job.csv";
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
        workers[n_workers]: Worker {
            parameters:
                test_tensor_size = default(26214400);
        };
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
