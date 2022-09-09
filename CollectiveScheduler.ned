moduleinterface ICollectiveScheduler
{
    gates:
        inout worker_ports[];
}

simple Sincronia like ICollectiveScheduler
{
    gates:
        inout worker_ports[];
}

simple ByteScheduler like ICollectiveScheduler
{
    gates:
        inout worker_ports[];
}

simple FifoExclusive like ICollectiveScheduler
{
    gates:
        inout worker_ports[];
}

simple ReadyAndGo like ICollectiveScheduler
{
    gates:
        inout worker_ports[];
}
