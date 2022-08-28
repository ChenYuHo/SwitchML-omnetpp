import ned.DelayChannel;

simple ClusterManager
{
}

channel Channel extends ned.DatarateChannel
{
    datarate = 100Gbps;
    delay = 1us;
    // packet error rate (corrupted transmission)
    per = 1e-10;
}

network TwoLayers
{
    parameters:
        int switch_ports = default(3);
        int n_workers = switch_ports*(switch_ports-1);
        **.NUM_SLOTS = default(256);
        **.NUM_UPDATES = default(256);
    submodules:
        switches[switch_ports+1]: Switch {
            parameters:
                pkRate = 1;
                queueMaxLen = 20; // buffer max 20 packets
            gates:
                port[parent.switch_ports];
        }
        workers[n_workers]: Worker {
            parameters:
                processingTime = 1s;
        }
        // in charge of job scheduling, job placement, collective scheduling
        cluster_manager: ClusterManager;
    connections allowunconnected:
        // ToR to Core
        for i=1..switch_ports {   
            switches[i].port[0] <--> Channel <--> switches[0].port[i-1];
        }
        // Worker to ToR
        for i=0..n_workers-1 {
            workers[i].port <--> Channel <--> switches[int(i/(switch_ports-1)+1)].port[i%(switch_ports-1)+1];
        }
}
