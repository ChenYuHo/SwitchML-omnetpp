simple Worker
{
    parameters:
        @signal[pktOut](type=cPacket);
        bool packet_simulation = default(true);
        int MTU @mutable = default(0);
        int num_slots = default(512);
        int num_updates @mutable = default(0);
        int num_gpus = default(4);
        @display("i=device/server");
    gates:
        inout port;
        input jobin;
        input directin;
}

simple TrainingProcess
{
    parameters:
        @signal[fullIterTime](type=simtime_t); // per iter, from first FP start to last WU end
        @signal[contiguousIterTime](type=simtime_t); // per iter, from first FP of this iter start to first FP of the next iter start
        @signal[minIdleTime](type=simtime_t); // per iter, time when no FP, BP is ongoing
        @signal[minIdleTimeWu](type=simtime_t); // per iter, time when no FP, BP, WU is ongoing
        @signal[idleTime](type=simtime_t); // per iter, time when no FP, BP is ongoing
        @signal[idleTimeWu](type=simtime_t); // per iter, time when no FP, BP, WU is ongoing
        @signal[commTime](type=simtime_t); // per iter, from first collective start to last collective end of an iteration
        @signal[realCommTime](type=simtime_t); // per iter, time spend for communication
		@signal[workerJobCompletionTime](type=simtime_t);
    gates:
        input directin;
}
