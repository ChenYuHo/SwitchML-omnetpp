simple Worker
{
    parameters:
        @signal[pktOut](type=cPacket);
        @signal[pktRetransmission](type=cPacket);
        bool packet_simulation = default(true);
        int MTU @mutable = default(0);
        int num_slots = default(512);
        int num_updates @mutable = default(0);
        int num_gpus = default(4);
        int retransmission_timeout @unit(ms) = default(0ms);
        string gpu_type = default("a100");
        @display("i=device/server");
    gates:
        inout port;
        input jobin;
        input directin;
}

simple TrainingProcess
{
    parameters:
        bool print_only_rank0 = default(false);
        double compress_probability = default(0);
        @signal[fullIterTime](type=simtime_t); // per iter, from first FP start to last WU end
        @signal[contiguousIterTime](type=simtime_t); // per iter, from first FP of this iter start to first FP (first WU ends) of the next iter start
        //        @signal[minIdleTime](type=simtime_t); // per iter, time when no FP, BP is ongoing
        //        @signal[minIdleTimeWu](type=simtime_t); // per iter, time when no FP, BP, WU is ongoing
        @signal[idleTime](type=simtime_t); // per iter, time when no FP, BP is ongoing
        @signal[idleTimeWu](type=simtime_t); // per iter, time when no FP, BP, WU is ongoing
        @signal[commTime](type=simtime_t); // per iter, from first collective start to last collective end of an iteration
        @signal[realCommTime](type=simtime_t); // per iter, time spend for communication
        @signal[workerJobCompletionTime](type=simtime_t); // actual execution times for a job (per worker)
    gates:
        input directin;
}
