simple Worker
{
    parameters:
        int MTU = default(0);
        int num_slots = default(512);
        int num_updates = default(0);
        int num_gpus = default(4);
        int test_tensor_size = default(26214400);
        @display("i=device/server");
    gates:
        inout port;
        input jobin;
        input directin;
}

simple TrainingProcess
{
//    parameters:
//        @signal[iterTime](type=simtime_t);
//        @statistic[iterTime](source=iterTime; record=histogram,vector);
//        
//        @signal[idleTime](type=simtime_t);
//        @statistic[idleTime](source=idleTime; record=histogram,vector);
//        
//        @signal[commTime](type=simtime_t);
//        @statistic[commTime](source=commTime; record=histogram,vector);
    gates:
        input directin;
}

simple TrainingProcess2
{
//    parameters:
//        @signal[iterTime](type=simtime_t);
//        @statistic[iterTime](source=iterTime; record=histogram,vector);
//        
//        @signal[idleTime](type=simtime_t);
//        @statistic[idleTime](source=idleTime; record=histogram,vector);
//        
//        @signal[commTime](type=simtime_t);
//        @statistic[commTime](source=commTime; record=histogram,vector);
    gates:
        input directin;
}
