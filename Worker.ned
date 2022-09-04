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
    gates:
        input directin;
}

//simple Allreducer
//{
//    parameters:
//        int chunk_size=0; 
//    gates:
//        input in;
//}
