import ned.DatarateChannel;

simple Switch
{
    parameters:
        @display("i=block/switch;q=queue");
    gates:
        inout up_ports[];
        inout down_ports[];
        input directin;
}

channel Channel extends DatarateChannel
{
    datarate = 100Gbps;
    delay = 1us;
    // packet error rate (corrupted transmission)
    per = 0;//1e-10;
}
