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
        inout job_port;
        inout cs_port;
        input directin;
}

simple TrainingProcess
{
    gates:
        input directin;
}
