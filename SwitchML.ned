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
                down_ports[parent.switch_ports];
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
            workers[i].port <--> Channel <--> tor.down_ports[i];
        }
        job_submitter.out --> job_dispatcher.in;
}

network TwoLayers
{
    parameters:
        **.num_updates = 256;
        **.num_slots = 512;
        int switch_ports = default(2);
        int n_workers = switch_ports*(switch_ports-1);
        job_submitter.max_jobs_to_submit = 2;
        collective_scheduler.typename = "";
        job_submitter.file = "1.csv";
    submodules:
        core: Switch {
            gates:
                down_ports[parent.switch_ports];
        }

        tors[switch_ports]: Switch {
            gates:
                up_ports[1];
                down_ports[parent.switch_ports-1];
        }
        workers[n_workers]: Worker {
            parameters:
                test_tensor_size = default(100);
        };
        job_submitter: <default("CSVJobSubmitter")> like IJobSubmitter;
        job_dispatcher: JobDispatcher;
        collective_scheduler: <default("Sincronia")> like ICollectiveScheduler if typename!="";
    connections allowunconnected:
        // ToR to Core
        for i=0..switch_ports-1 {   
            tors[i].up_ports[0] <--> Channel <--> core.down_ports[i];
        }
        // Worker to ToR
        for i=0..n_workers-1 {
            workers[i].port <--> Channel <--> tors[int(i/(switch_ports-1))].down_ports[i%(switch_ports-1)];
        }
        job_submitter.out --> job_dispatcher.in;
}
