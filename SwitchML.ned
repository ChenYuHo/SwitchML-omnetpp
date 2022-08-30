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
        string file = "60_job.csv";
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
}

moduleinterface ICollectiveScheduler
{
}

simple Sincronia like ICollectiveScheduler
{
}

//moduleinterface IJobScheduler
//{
//    gates:
//        input in;
//        output out; // to Job Placer
//}
//
//simple FifoJobScheduler like IJobScheduler
//{
//    gates:
//        input in;
//        output out; // to Job Placer
//}

//moduleinterface IJobPlacer
//{
////    gates:
////        input in; // from Job Scheduler
////        output out[]; // to Workers
//}
//
//simple RandomJobPlacer like IJobPlacer
//{
////    gates:
////        input in; // from Job Scheduler
////        output out[]; // to Workers
//}

network TwoLayers
{
    parameters:
        **.num_updates = 10;
        **.num_slots = 2;
        int switch_ports = default(3);
        int n_workers = switch_ports*(switch_ports-1);
        job_submitter.max_jobs_to_submit = 2;
        collective_scheduler.typename = "";
    //        **.NUM_SLOTS = default(256);
    //        **.NUM_UPDATES = default(256);
    submodules:
//        core: Spine_ToR {
//            parameters:
//                n_uplinks = 0;
//                tors[*].n_uplinks = 1;
//                n_downlinks = parent.switch_ports;
//                tors[*].n_downlinks = parent.switch_ports - 1;
//                n_ports = parent.switch_ports;
//                tors[*].n_ports = parent.switch_ports;
//        }

//        tors[switch_ports]: ToR {
//            parameters:
//                n_downlink = switch_ports-1;
//            gates:
//                port[parent.switch_ports];
//        }
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
        // in charge of job scheduling, job placement, collective scheduling
        //        cluster_manager: ClusterManager;
        job_submitter: <default("CSVJobSubmitter")> like IJobSubmitter;
        job_dispatcher: JobDispatcher;
        //        job_scheduler: <default("FifoJobScheduler")> like IJobScheduler;
        //        job_placer: <default("RandomJobPlacer")> like IJobPlacer;
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
        
        
        //        for i=0..switch_ports {
        //            core.port++ <--> Channel <--> core.tors[i].port++;
        //            
        //            for j=0..switch_ports-1 {
        //                core.tors[i].port++ <--> Channel <--> core.tors[i].workers[j].port;
        //            }
        //            
        //        }
        job_submitter.out --> job_dispatcher.in;



}
