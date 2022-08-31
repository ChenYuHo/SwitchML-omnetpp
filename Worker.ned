simple Worker
{
    parameters:
        int num_slots;
        int num_updates;
        int num_gpus = default(4);
        double processingTime @unit(s) @prompt("Time to process a query") = default(0.2s);
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
