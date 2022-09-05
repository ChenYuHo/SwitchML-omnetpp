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

simple Fifo like ICollectiveScheduler
{
    parameters:
        @class(Fifo_CS);
    gates:
        input directin;
}
