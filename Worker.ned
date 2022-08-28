simple Worker
{
    parameters:
        double processingTime @unit(s) @prompt("Time to process a query") = default(0.2s);
        @display("i=device/server");
    gates:
        inout port;
        inout msg;
}

simple TrainingProcess
{
    gates:
        input in;
}

simple AllReducer
{
    parameters:
        int chunk_size; 
    gates:
        input in;
}
