import ned.DatarateChannel;
import ned.DelayChannel;
import ned.IdealChannel;

network mcnodes
{
    parameters:
        @statistic[nTimeouts](source=count(pktRetransmission); record=last); // from all workers
        @statistic[txBits](source=packetBits(pktOut); record=sum); // from all workers
        @statistic[maxWorkerQueueLength](source=workerQueueLength; record=max); // from all workers
        @statistic[maxSwitchQueueLength](source=switchQueueLength; record=max); // from all switches
        @statistic[maxSwitchTotalQueueLength](source=switchTotalQueueLength; record=max); // from all switches
        // @statistic[rxBits](source=sum(packetBits(pktIn)); record=last); // ignored because for SwitchML it will be the same as txBits
        @statistic[totalCommTime](source=commTime; record=sum); // time can do first comm until last comm done (per iter, for all workers)
        @statistic[totalRealCommTime](source=realCommTime; record=sum); // time starting every SwitchML transmission until its end (per iter, for all workers)
        // totalDelayTime is totalCommTime - totalRealCommTime
        @statistic[totalDelayCommTime](source=sum(commTime)-sum(realCommTime); record=last);
        // network utilization is txBits / totalCommTime
        @statistic[avgBandwidth](source=sum(packetBits(pktOut))/sum(commTime)/1000000000; record=last);
        // @statistic[realBandwidth](source=sum(packetBits(pktOut))/sum(realCommTime)/1000000000; record=last);

//        @statistic[totalMinIdleTimeWu](source=minIdleTimeWu; record=sum);
//        @statistic[totalMinIdleTime](source=minIdleTime; record=sum);
        @statistic[totalIdleTimeWu](source=idleTimeWu; record=sum);
        @statistic[totalIdleTime](source=idleTime; record=sum); // waiting time for each fp's dependent (bp and wu) updates
        @statistic[totalWorkerJobCompletionTime](source=workerJobCompletionTime; record=sum); // per worker

        // gpuUtilization is 1 - (totalIdleTime / totalWorkerJobCompletionTime)
        @statistic[gpuUtilization](source=1-sum(idleTime)/sum(workerJobCompletionTime); record=last);
        @statistic[gpuUtilizationWu](source=1-sum(idleTimeWu)/sum(workerJobCompletionTime); record=last);

        // scheduling efficiency is sum of min idle time / sum of idle time
//        @statistic[schedulingEfficiency](source=sum(minIdleTime)/sum(idleTime); record=last); // sum of min idle time / sum of idle time
//        @statistic[schedulingEfficiencyWu](source=sum(minIdleTimeWu)/sum(idleTimeWu); record=last);

        @statistic[jobCompletionTime](record=stats,vector); // from first worker start to last worker finish for a job
        @statistic[jctInflation](record=stats,vector); // from first worker start to last worker finish for a job
        @statistic[jobSubmissionTime](record=vector);
        @statistic[jobWaitTime](record=stats,vector);
        @statistic[jobStartTime](record=vector);
        @statistic[jobPlacementType](record=vector);

        @statistic[compressedSize](record=sum,stats,histogram);
        @statistic[uncompressedSize](record=sum,stats,histogram);
        int switch_ports = default(4);
        int num_tors = default(2);
        int n_workers = switch_ports*num_tors;
    types:
        channelinterface IMyChannel
        {
        }
        channel Ideal extends ned.IdealChannel like IMyChannel
        {
        }
        channel Delay extends ned.DelayChannel like IMyChannel
        {
        }
        channel Datarate extends ned.DatarateChannel like IMyChannel
        {
        }
    submodules:
        core: Switch {
            gates:
                down_ports[parent.num_tors];
        }
        tors[num_tors]: Switch {
            gates:
                up_ports[1];
                down_ports[parent.switch_ports];
        }
        workers[n_workers]: Worker;
        job_submitter: <default("CSVJobSubmitter")> like IJobSubmitter;
        job_dispatcher: JobDispatcher;
        collective_scheduler: <default("Sincronia")> like ICollectiveScheduler if typename!="None";
    connections allowunconnected:
        // ToR to Core
        for i=0..num_tors-1 {
            tors[i].up_ports[0] <--> tor_core_channel: <default("Datarate")> like IMyChannel <--> core.down_ports[i];
        }
        // Worker to ToR
        for i=0..n_workers-1 {
            workers[i].port <--> tor_worker_channel: <default("Datarate")> like IMyChannel <--> tors[int(i / switch_ports)].down_ports[i % switch_ports];
        }
        job_submitter.jobout --> job_dispatcher.jobin;
}

network TwoLayers
{
    parameters:
        @statistic[nTimeouts](source=count(pktRetransmission); record=last); // from all workers
        @statistic[txBits](source=packetBits(pktOut); record=sum); // from all workers
        @statistic[maxWorkerQueueLength](source=workerQueueLength; record=max); // from all workers
        @statistic[maxSwitchQueueLength](source=switchQueueLength; record=max); // from all switches
        @statistic[maxSwitchTotalQueueLength](source=switchTotalQueueLength; record=max); // from all switches
        // @statistic[rxBits](source=sum(packetBits(pktIn)); record=last); // ignored because for SwitchML it will be the same as txBits
        @statistic[totalCommTime](source=commTime; record=sum); // time can do first comm until last comm done (per iter, for all workers)
        @statistic[totalRealCommTime](source=realCommTime; record=sum); // time starting every SwitchML transmission until its end (per iter, for all workers)
        // totalDelayTime is totalCommTime - totalRealCommTime
        @statistic[totalDelayCommTime](source=sum(commTime)-sum(realCommTime); record=last);
        // network utilization is txBits / totalCommTime
        @statistic[avgBandwidth](source=sum(packetBits(pktOut))/sum(commTime)/1000000000; record=last);
        // @statistic[realBandwidth](source=sum(packetBits(pktOut))/sum(realCommTime)/1000000000; record=last);

//        @statistic[totalMinIdleTimeWu](source=minIdleTimeWu; record=sum);
//        @statistic[totalMinIdleTime](source=minIdleTime; record=sum);
        @statistic[totalIdleTimeWu](source=idleTimeWu; record=sum);
        @statistic[totalIdleTime](source=idleTime; record=sum); // waiting time for each fp's dependent (bp and wu) updates
        @statistic[totalWorkerJobCompletionTime](source=workerJobCompletionTime; record=sum); // per worker

        // gpuUtilization is 1 - (totalIdleTime / totalWorkerJobCompletionTime)
        @statistic[gpuUtilization](source=1-sum(idleTime)/sum(workerJobCompletionTime); record=last);
        @statistic[gpuUtilizationWu](source=1-sum(idleTimeWu)/sum(workerJobCompletionTime); record=last);

        // scheduling efficiency is sum of min idle time / sum of idle time
//        @statistic[schedulingEfficiency](source=sum(minIdleTime)/sum(idleTime); record=last); // sum of min idle time / sum of idle time
//        @statistic[schedulingEfficiencyWu](source=sum(minIdleTimeWu)/sum(idleTimeWu); record=last);

        @statistic[jobCompletionTime](record=stats,vector); // from first worker start to last worker finish for a job
        @statistic[jctInflation](record=stats,vector); // from first worker start to last worker finish for a job
        @statistic[jobSubmissionTime](record=vector);
        @statistic[jobWaitTime](record=stats,vector);
        @statistic[jobStartTime](record=vector);
        @statistic[jobPlacementType](record=vector);

        @statistic[compressedSize](record=sum,stats,histogram);
        @statistic[uncompressedSize](record=sum,stats,histogram);
        int switch_ports = default(16);
        int n_workers = switch_ports*(switch_ports-1);
    types:
        channelinterface IMyChannel
        {
        }
        channel Ideal extends ned.IdealChannel like IMyChannel
        {
        }
        channel Delay extends ned.DelayChannel like IMyChannel
        {
        }
        channel Datarate extends ned.DatarateChannel like IMyChannel
        {
        }
    submodules:
        core: Switch {
            gates:
                down_ports[parent.switch_ports];
        }
        tors[switch_ports]: Switch {
            gates:
                up_ports[1];
                down_ports[parent.switch_ports-1];
        }
        workers[n_workers]: Worker;
        job_submitter: <default("CSVJobSubmitter")> like IJobSubmitter;
        job_dispatcher: JobDispatcher;
        collective_scheduler: <default("Sincronia")> like ICollectiveScheduler if typename!="None";
    connections allowunconnected:
        // ToR to Core
        for i=0..switch_ports-1 {
            tors[i].up_ports[0] <--> tor_core_channel: <default("Datarate")> like IMyChannel <--> core.down_ports[i];
        }
        // Worker to ToR
        for i=0..n_workers-1 {
            workers[i].port <--> tor_worker_channel: <default("Datarate")> like IMyChannel <--> tors[int(i/(switch_ports-1))].down_ports[i%(switch_ports-1)];
        }
        job_submitter.jobout --> job_dispatcher.jobin;
}
