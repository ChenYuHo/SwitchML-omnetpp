moduleinterface ICollectiveScheduler
{
    gates:
        input directin;
}

simple Sincronia like ICollectiveScheduler
{
    gates:
        input directin;
}

simple ByteScheduler like ICollectiveScheduler
{
    gates:
        input directin;
}

simple FifoExclusive like ICollectiveScheduler
{
    gates:
        input directin;
}

simple ReadyAndGo like ICollectiveScheduler
{
    gates:
        input directin;
}
