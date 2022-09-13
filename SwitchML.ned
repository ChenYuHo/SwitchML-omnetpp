import ned.DatarateChannel;
import ned.DelayChannel;
import ned.IdealChannel;



network TwoLayers
{
    parameters:
        int switch_ports = default(16);
        int n_workers = switch_ports*(switch_ports-1);
    types:
        channelinterface IMyChannel
        {
        }
        channel Ideal extends ned.IdealChannel like IMyChannel
        {
        }
        channel Delay extends ned.DelayChannel like IMyChannel
        {
        }
        channel Datarate extends ned.DatarateChannel like IMyChannel
        {
        }
    submodules:
//        core_queues_down[switch_ports]: L2Queue;
        core: Switch {
            gates:
                down_ports[parent.switch_ports];
        }
//        tor_queues_up[switch_ports]: L2Queue;
//        tor_queues_down[n_workers]: L2Queue;
        tors[switch_ports]: Switch {
            gates:
                up_ports[1];
                down_ports[parent.switch_ports-1];
        }
//        worker_queues_up[n_workers]: L2Queue;
        workers[n_workers]: Worker;
        job_submitter: <default("CSVJobSubmitter")> like IJobSubmitter;
        job_dispatcher: JobDispatcher;
        collective_scheduler: <default("Sincronia")> like ICollectiveScheduler if typename!="None";
    connections allowunconnected:
        // ToR to Core
        for i=0..switch_ports-1 {
//            tors[i].up_ports[0] <--> tor_queues_up[i].inside;
            tors[i].up_ports[0] <--> tor_core_channel: <default("Datarate")> like IMyChannel <--> core.down_ports[i];
//            tor_queues_up[i].outside <--> tor_core_channel: <default("Datarate")> like IMyChannel <--> core_queues_down[i].outside;
//            core_queues_down[i].inside <--> core.down_ports[i];
        }
        // Worker to ToR
        for i=0..n_workers-1 {
//            workers[i].port <--> worker_queues_up[i].inside;
            workers[i].port <--> tor_worker_channel: <default("Datarate")> like IMyChannel <--> tors[int(i/(switch_ports-1))].down_ports[i%(switch_ports-1)];
//            tor_queues_down[i].inside <--> tors[int(i/(switch_ports-1))].down_ports[i%(switch_ports-1)];
        }
        job_submitter.jobout --> job_dispatcher.jobin;
}
