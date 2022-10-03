simple Worker
{
    parameters:
        @signal[testSignal](type=bool);
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
        @signal[idleTime](type=simtime_t); // per iter, time when no FP, BP is ongoing
        @signal[idleTimeWu](type=simtime_t); // per iter, time when no FP, BP, WU is ongoing
        @signal[commTime](type=simtime_t); // per iter, from first collective start to last collective end of an iteration
        @signal[realCommTime](type=simtime_t); // per iter, time spend for communication
        @statistic[fullIterTime](record=vector?);
        @statistic[contiguousIterTime](record=vector?);
        @statistic[idleTime](record=vector?);
        @statistic[idleTimeWu](record=vector?);
        @statistic[commTime](record=vector?);
        @statistic[realCommTime](record=vector?);
        @statistic[totalIdleTime](source=idleTime; record=sum,vector?);
        @statistic[totalIdleTimeWu](source=idleTime; record=sum,vector?);
    gates:
        input directin;
}
