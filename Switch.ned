import ned.DatarateChannel;

simple Switch
{
    parameters:
        double pkRate = 1;
        int queueMaxLen = 1;
        @display("i=block/switch;q=queue");
    gates:
        inout port_up[];
        inout port_down[];
}

channel Channel extends DatarateChannel
{
    datarate = 100Gbps;
    delay = 1us;
    // packet error rate (corrupted transmission)
    per = 0;//1e-10;
}
