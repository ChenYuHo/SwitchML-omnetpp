simple Worker
{
    parameters:
        int num_slots;
        int num_updates;
        int num_gpus = default(4);
        int test_tensor_size = default(26214400);
        @display("i=device/server");
    gates:
        inout port;
        input jobin;
}

simple TrainingProcess
{
    gates:
        input in;
}

simple Allreducer
{
    parameters:
        int chunk_size=0; 
    gates:
        input in;
}
