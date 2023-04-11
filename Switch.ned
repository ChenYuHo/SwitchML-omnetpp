simple Switch
{
    parameters:
        @signal[switchQueueLength](type=int);
        @display("i=block/switch;q=queue");
    gates:
        inout up_ports[];
        inout down_ports[];
        input directin;
}
