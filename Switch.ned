simple Switch
{
    parameters:
        @display("i=block/switch;q=queue");
    gates:
        inout up_ports[];
        inout down_ports[];
        input directin;
}
