moduleinterface ICollectiveScheduler
{
    parameters:
        int chunk_size;
    gates:
        input directin;
}

simple Sincronia like ICollectiveScheduler
{
    parameters:
        int chunk_size = default(262144);
        bool exclusive = false;
        bool compression = default(false);
        string weighting_fn = default("none");
        @signal[compressedSize](type=int);
        @signal[uncompressedSize](type=int);
    gates:
        input directin;
}

simple SincroniaExclusive like ICollectiveScheduler
{
    parameters:
        int chunk_size = default(262144);
        bool exclusive = true;
        @class(Sincronia);
    gates:
        input directin;
}

simple DeficitRoundRobin like ICollectiveScheduler
{
    parameters:
        int chunk_size = default(262144);
        bool exclusive = false;
    gates:
        input directin;
}

simple DeficitRoundRobinExclusive like ICollectiveScheduler
{
    parameters:
        int chunk_size = default(262144);
        bool exclusive = true;
        @class(DeficitRoundRobin);
    gates:
        input directin;
}

simple ByteScheduler like ICollectiveScheduler
{
    parameters:
        double compress_probability = default(0);
        int chunk_size = default(262144);
    gates:
        input directin;
}

simple FifoExclusive like ICollectiveScheduler
{
    parameters:
        int chunk_size = default(0);
    gates:
        input directin;
}

simple ReadyAndGo like ICollectiveScheduler
{
    parameters:
        int chunk_size = default(0);
    gates:
        input directin;
}
