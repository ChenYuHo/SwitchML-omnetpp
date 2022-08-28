simple Switch
{
    parameters:
        double pkRate;
        int queueMaxLen;
        @display("i=block/switch;q=queue");
    gates:
        inout port[];
}
