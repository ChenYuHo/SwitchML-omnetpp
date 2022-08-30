import ned.DatarateChannel;

simple Switch
{
    parameters:
//        int n_ports;
//        int n_uplinks;
//        int n_downlinks;
        double pkRate = 1;
        int queueMaxLen = 1;
        @display("i=block/switch;q=queue");
    gates:
        inout port_up[];
        inout port_down[];
}

channel Channel extends DatarateChannel
{
    datarate = 100Gbps;
    delay = 1us;
    // packet error rate (corrupted transmission)
    per = 0;//1e-10;
}


//module ToR extends Switch
//{
//    submodules:
//        workers[n_downlinks]: Worker;
//    connections allowunconnected:
//        for i=0..n_downlinks-1{
//            workers[i].port <--> Channel <--> port[i+n_uplinks];
//        }
//        
//}

//module Spine_ToR extends Switch
//{
//    submodules:
//		tors[n_downlinks]: ToR;
//	connections allowunconnected:
//	    for i=0..n_downlinks-1 {
//	        tors[i].port[0] <--> Channel <--> port[i+n_uplinks];
//	    }
//}

//module Spine extends Switch
//{
//    submodules:
//        switches[n_downlinks]: Switch;
//    connections allowunconnected:
//        for i=0..n_downlinks-1 {
//	        switches[i].port[0] <--> Channel <--> port[i+n_uplinks];
//	    }
//}