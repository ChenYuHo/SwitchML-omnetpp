import ned.DatarateChannel;

network TwoLayers
{
    parameters:
        **.channel.datarate = 10Gbps;
        **.channel.delay = 1us;
        **.MTU = 1500;
        **.num_updates = 256;
        **.num_slots = 128;
        int switch_ports = default(2);
        int n_workers = switch_ports*(switch_ports-1);
        job_submitter.max_jobs_to_submit = 2;
        collective_scheduler.typename = "Fifo";
        job_submitter.file = "1.csv";
    submodules:
        core_queues_down[switch_ports]: L2Queue;
        core: Switch {
            gates:
                down_ports[parent.switch_ports];
        }
        tor_queues_up[switch_ports]: L2Queue;
        tor_queues_down[n_workers]: L2Queue;
        tors[switch_ports]: Switch {
            gates:
                up_ports[1];
                down_ports[parent.switch_ports-1];
        }
        worker_queues_up[n_workers]: L2Queue;
        workers[n_workers]: Worker {
            parameters:
                test_tensor_size = default(100);
        }
        job_submitter: <default("CSVJobSubmitter")> like IJobSubmitter;
        job_dispatcher: JobDispatcher;
        collective_scheduler: <default("Sincronia")> like ICollectiveScheduler if typename!="none";
    connections allowunconnected:
        // ToR to Core
        for i=0..switch_ports-1 {
            tors[i].up_ports[0] <--> tor_queues_up[i].inside;
            tor_queues_up[i].outside <--> DatarateChannel <--> core_queues_down[i].outside;
            core_queues_down[i].inside <--> core.down_ports[i];
        }
        // Worker to ToR
        for i=0..n_workers-1 {
            workers[i].port <--> worker_queues_up[i].inside;
            worker_queues_up[i].outside <--> DatarateChannel <--> tor_queues_down[i].outside;
            tor_queues_down[i].inside <--> tors[int(i/(switch_ports-1))].down_ports[i%(switch_ports-1)];
        }
        job_submitter.out --> job_dispatcher.in;
}
