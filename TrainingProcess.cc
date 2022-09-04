#include "TrainingProcess.h"
#include "ModelStats.h"
#include "Worker.h"

Define_Module(TrainingProcess);

void TrainingProcess::allreduce(Job *job, uint64_t layer, uint64_t size, uint64_t iter) {
    auto req = new CollectiveOperationRequest();
    req->setKind(0);
    req->setTraining_process_id(getId());
    req->setWorker_id(worker->getId());
    req->setModel(job->getModel());
    req->setSize(size);
    req->setRank(job->getRank());
    req->setLayer(layer);
    req->setTensor_key(
            hasher(fmt::format("jid{}tid{}iter{}", job->getJob_id(), layer, iter)));
    req->setJob_id(job->getJob_id());
    req->setNum_workers_allocated(job->getNum_workers_allocated());
    if (collective_scheduler) {
        EV << "Enqueue Allreduce" << endl;
        sendDirect(req, collective_scheduler, "directin");
    } else { // send directly to Worker
        EV_DEBUG << fmt::format("TrainingProcess start allreduce for job {} layer {} size {} iter {}\n", job->getJob_id(), layer, size, iter);
        sendDirect(req, getParentModule(), "directin");
    }
}

//void TrainingProcess::forward_ack(LayerAck *ack) {
//    Enter_Method_Silent();
//    // notify allreducer
//    ack->setWeight_update_time(model[ack->getLayer()]);
////    sendDirect(ack, allreducer, "in");
//}

void TrainingProcess::process_ack(LayerAck *ack) {
    can_do_fp[ack->getLayer()] = true;
    delete ack;
}

void TrainingProcess::waitAndProcessAck(simtime_t wait_time, cQueue *AckQueue) {
    waitAndEnqueue(wait_time, AckQueue);
    while (!AckQueue->isEmpty()) {
        process_ack(check_and_cast<LayerAck*>(AckQueue->pop()));
    }
}

void TrainingProcess::activity() {
    // retrieve parameters
//    auto srvProcType = cModuleType::get("Allreducer");
    collective_scheduler = getSimulation()->findModuleByPath("<root>.collective_scheduler");
    worker = (Worker*) getParentModule();
    if (collective_scheduler) {
        EV_DEBUG << "Collective Scheduler is " << collective_scheduler->getFullName()
                  << endl;
    } else
        EV_DEBUG << "No Collective Scheduler" << endl;
    auto job = check_and_cast<Job*>(receive());
//    setup(job);
    auto rank = job->getRank();
    auto jid = job->getJob_id();
    auto iters = job->getIters();

    auto model = job->getModel();
    auto num_layers = n_layers(model);

    bool distributed = job->getNum_workers_allocated() > 1;
//    if (distributed) {
//        allreducer = (Allreducer*) srvProcType->createScheduleInit("allreducer",
//                this);
//        EV << "TrainingProcess id " << getId() << " created allreducer "
//                  << allreducer->getName() << endl;
//    }
    can_do_fp.resize(num_layers, true);
    EV << fmt::format("Start Job {} as rank {} iters {} num_layers {}", jid, rank, iters, num_layers) << endl;
    cQueue AckQueue(fmt::format("Allreducer{}", getId()).c_str());

    for (unsigned iter = 0; iter < iters; ++iter) {
        for (size_t layer = 0; layer < num_layers; ++layer) {
            while (!can_do_fp[layer]) {
                process_ack(check_and_cast<LayerAck*>(receive()));
            }
            waitAndProcessAck(SimTime(fp_times[model][layer], SIMTIME_PS), &AckQueue);
            can_do_fp[layer] = false;
        }

        for (int layer = num_layers - 1; layer >= 0; --layer) {
            waitAndProcessAck(SimTime(bp_times[model][layer], SIMTIME_PS), &AckQueue);
            if (distributed) {
                allreduce(job, layer, model_sizes[model][layer], iter);
            } else {
                auto ack = new LayerAck();
                ack->setLayer(layer);
                scheduleAfter(SimTime(wu_times[model][layer], SIMTIME_PS), ack);
            }
        }
    }

    for (size_t i = 0; i < num_layers; ++i) {
        while (!can_do_fp[i]) {
            process_ack(check_and_cast<LayerAck*>(receive()));
        }
    }

    EV << fmt::format("rank {} done job {} at {}\n", rank, jid, simTime().raw());
    job->setFinish_time(simTime());
//    auto job_dispatcher = this->getSimulation()->getModuleByPath(
//            "<root>.job_dispatcher");
//    this->sendDirect(job->dup(), job_dispatcher, "directin");
    job->setKind(5);
    this->sendDirect(job, getParentModule(), "directin");
    deleteModule();
}

//void TrainingProcess::setup(Job *job) {
//    n_workers = job->getNum_workers_allocated();
//    auto m = job->getModel();
//    if (m == "alexnet") {
//        model = vector<uint64_t>( { 330688, 39891840, 16781312, 4097000 });
//        forward_pass_time = vector<simtime_t> { SimTime(6487422000, SIMTIME_PS),
//                SimTime(5547996500, SIMTIME_PS), SimTime(482768500, SIMTIME_PS),
//                SimTime(254498000, SIMTIME_PS) };
//        backward_pass_time = vector<simtime_t> { SimTime(6780418500,
//                SIMTIME_PS), SimTime(7897807000, SIMTIME_PS), SimTime(791112000,
//                SIMTIME_PS), SimTime(538649500, SIMTIME_PS) };
//        weight_update_time = vector<simtime_t> { SimTime(1167345, SIMTIME_PS),
//                SimTime(140820177, SIMTIME_PS), SimTime(59238865, SIMTIME_PS),
//                SimTime(14462613, SIMTIME_PS) };
//    } else if (m == "vgg11") {
//        model =
//                vector<uint64_t> { 370816, 8849664, 102764544, 16781312, 4097000 };
//        forward_pass_time = vector<simtime_t> { SimTime(45970975500,
//                SIMTIME_PS), SimTime(36164820500, SIMTIME_PS), SimTime(
//                4127456500, SIMTIME_PS), SimTime(485374000, SIMTIME_PS),
//                SimTime(287803500, SIMTIME_PS) };
//        backward_pass_time = vector<simtime_t> { SimTime(91740694000,
//                SIMTIME_PS), SimTime(50867676500, SIMTIME_PS), SimTime(
//                4109033500, SIMTIME_PS), SimTime(795572000, SIMTIME_PS),
//                SimTime(538523000, SIMTIME_PS) };
//        weight_update_time = vector<simtime_t> { SimTime(724217, SIMTIME_PS),
//                SimTime(17283720, SIMTIME_PS), SimTime(200702948, SIMTIME_PS),
//                SimTime(32774522, SIMTIME_PS), SimTime(8001592, SIMTIME_PS) };
//    } else if (m == "vgg16") {
//        model = vector<uint64_t> { 555328, 7079936, 7079424, 102764544,
//                16781312, 4097000 };
//        forward_pass_time =
//                vector<simtime_t> { SimTime(90816362000, SIMTIME_PS), SimTime(
//                        51354924000, SIMTIME_PS), SimTime(10024820000,
//                        SIMTIME_PS), SimTime(4156633000, SIMTIME_PS), SimTime(
//                        478786000, SIMTIME_PS), SimTime(287318500, SIMTIME_PS) };
//        backward_pass_time =
//                vector<simtime_t> { SimTime(200656557000, SIMTIME_PS), SimTime(
//                        81372176000, SIMTIME_PS), SimTime(15606738500,
//                        SIMTIME_PS), SimTime(4108847000, SIMTIME_PS), SimTime(
//                        796019000, SIMTIME_PS), SimTime(550236500, SIMTIME_PS) };
//        weight_update_time = vector<simtime_t> { SimTime(1287480, SIMTIME_PS),
//                SimTime(16414219, SIMTIME_PS), SimTime(16413032, SIMTIME_PS),
//                SimTime(238250708, SIMTIME_PS), SimTime(38906021, SIMTIME_PS),
//                SimTime(9498540, SIMTIME_PS) };
//    } else if (m == "vgg19") {
//        model = vector<uint64_t> { 555328, 7670016, 7079424, 107484160,
//                16781312, 4097000 };
//        forward_pass_time =
//                vector<simtime_t> { SimTime(90902865500, SIMTIME_PS), SimTime(
//                        63746658500, SIMTIME_PS), SimTime(16688510000,
//                        SIMTIME_PS), SimTime(10410244000, SIMTIME_PS), SimTime(
//                        484357500, SIMTIME_PS), SimTime(293821000, SIMTIME_PS) };
//        backward_pass_time =
//                vector<simtime_t> { SimTime(200714238500, SIMTIME_PS), SimTime(
//                        112808855000, SIMTIME_PS), SimTime(20643356500,
//                        SIMTIME_PS), SimTime(13684955500, SIMTIME_PS), SimTime(
//                        799120000, SIMTIME_PS), SimTime(568673000, SIMTIME_PS) };
//        weight_update_time = vector<simtime_t> { SimTime(1386074, SIMTIME_PS),
//                SimTime(19144017, SIMTIME_PS), SimTime(17669925, SIMTIME_PS),
//                SimTime(268275652, SIMTIME_PS), SimTime(41885404, SIMTIME_PS),
//                SimTime(10225929, SIMTIME_PS) };
//    } else if (m == "inception" || m == "inception3" || m == "inception4") { // trace is inceptionv3
//        model = vector<uint64_t> { 271200, 6700160, 7604224, 6815616, 2443368 };
//        forward_pass_time = vector<simtime_t> { SimTime(39062640000,
//                SIMTIME_PS), SimTime(69619003000, SIMTIME_PS), SimTime(
//                23538588500, SIMTIME_PS), SimTime(10326569500, SIMTIME_PS),
//                SimTime(2301948000, SIMTIME_PS) };
//        backward_pass_time = vector<simtime_t> { SimTime(91082534500,
//                SIMTIME_PS), SimTime(157184949500, SIMTIME_PS), SimTime(
//                51105377500, SIMTIME_PS), SimTime(18621429000, SIMTIME_PS),
//                SimTime(4108938000, SIMTIME_PS) };
//        weight_update_time =
//                vector<simtime_t> { SimTime(23843606, SIMTIME_PS), SimTime(
//                        589070697, SIMTIME_PS), SimTime(668555009, SIMTIME_PS),
//                        SimTime(599221461, SIMTIME_PS), SimTime(214818227,
//                                SIMTIME_PS) };
//    } else if (m == "googlenet") {
//        model = vector<uint64_t> { 266368, 6358536 };
//        forward_pass_time = vector<simtime_t> { SimTime(14777889000,
//                SIMTIME_PS), SimTime(38211556500, SIMTIME_PS) };
//        backward_pass_time = vector<simtime_t> { SimTime(34875528000,
//                SIMTIME_PS), SimTime(72078958000, SIMTIME_PS) };
//        weight_update_time = vector<simtime_t> { SimTime(53534230, SIMTIME_PS),
//                SimTime(1277928770, SIMTIME_PS) };
//    } else if (m == "resnet101") {
//        model = vector<uint64_t> { 405824, 6755584, 6703104, 6703104, 6703104,
//                7352832, 6822912, 3102696 };
//        forward_pass_time = vector<simtime_t> { SimTime(36415028000,
//                SIMTIME_PS), SimTime(45010404000, SIMTIME_PS), SimTime(
//                24932418500, SIMTIME_PS), SimTime(25036425000, SIMTIME_PS),
//                SimTime(24930242000, SIMTIME_PS), SimTime(11107929000,
//                        SIMTIME_PS), SimTime(4911117500, SIMTIME_PS), SimTime(
//                        2078128500, SIMTIME_PS) };
//        backward_pass_time = vector<simtime_t> { SimTime(77078881500,
//                SIMTIME_PS), SimTime(83454325500, SIMTIME_PS), SimTime(
//                45548697000, SIMTIME_PS), SimTime(45538862500, SIMTIME_PS),
//                SimTime(45566164000, SIMTIME_PS), SimTime(23223537000,
//                        SIMTIME_PS), SimTime(8892756000, SIMTIME_PS), SimTime(
//                        3708969500, SIMTIME_PS) };
//        weight_update_time = vector<simtime_t> { SimTime(20383371, SIMTIME_PS),
//                SimTime(339313527, SIMTIME_PS), SimTime(336677608, SIMTIME_PS),
//                SimTime(336677608, SIMTIME_PS), SimTime(336677608, SIMTIME_PS),
//                SimTime(369311574, SIMTIME_PS), SimTime(342695219, SIMTIME_PS),
//                SimTime(155839484, SIMTIME_PS) };
//    } else if (m == "resnet152") {
//        model = vector<uint64_t> { 405824, 6758656, 6703104, 6703104, 6703104,
//                6703104, 6703104, 8534528, 7875584, 3102696 };
//        forward_pass_time = vector<simtime_t> { SimTime(36436733000,
//                SIMTIME_PS), SimTime(62309174000, SIMTIME_PS), SimTime(
//                25085074500, SIMTIME_PS), SimTime(24963430500, SIMTIME_PS),
//                SimTime(24960930000, SIMTIME_PS), SimTime(24952071500,
//                        SIMTIME_PS), SimTime(25011073000, SIMTIME_PS), SimTime(
//                        18011680500, SIMTIME_PS), SimTime(6339820500,
//                        SIMTIME_PS), SimTime(2082999000, SIMTIME_PS) };
//        backward_pass_time = vector<simtime_t> { SimTime(77132477500,
//                SIMTIME_PS), SimTime(113671785500, SIMTIME_PS), SimTime(
//                45698397000, SIMTIME_PS), SimTime(45704558000, SIMTIME_PS),
//                SimTime(45694686000, SIMTIME_PS), SimTime(45699517000,
//                        SIMTIME_PS), SimTime(45701750000, SIMTIME_PS), SimTime(
//                        35786120000, SIMTIME_PS), SimTime(11665839000,
//                        SIMTIME_PS), SimTime(3928979500, SIMTIME_PS) };
//        weight_update_time = vector<simtime_t> { SimTime(21999307, SIMTIME_PS),
//                SimTime(366379879, SIMTIME_PS), SimTime(363368461, SIMTIME_PS),
//                SimTime(363368461, SIMTIME_PS), SimTime(363368461, SIMTIME_PS),
//                SimTime(363368461, SIMTIME_PS), SimTime(363368461, SIMTIME_PS),
//                SimTime(462648097, SIMTIME_PS), SimTime(426927412, SIMTIME_PS),
//                SimTime(168194000, SIMTIME_PS) };
//    } else if (m == "bert") {
//        model = vector<uint64_t> { 31260672, 8927232, 7346176, 9445376, 8400896,
//                7346176, 9445376, 8400896, 7346176, 9445376, 8400896, 7346176,
//                9445376, 8400896, 7346176, 9445376, 8400896, 7346176, 9445376,
//                8400896, 7346176, 9445376, 8400896, 7346176, 9445376, 8400896,
//                7346176, 9445376, 8400896, 7346176, 9445376, 8400896, 7346176,
//                9445376, 8400896, 7346176, 9445376, 1053698 };
//        forward_pass_time = vector<simtime_t> { SimTime(201679500, SIMTIME_PS),
//                SimTime(1695811500, SIMTIME_PS), SimTime(613803500, SIMTIME_PS),
//                SimTime(977431500, SIMTIME_PS), SimTime(1360976000, SIMTIME_PS),
//                SimTime(592250500, SIMTIME_PS), SimTime(1002414000, SIMTIME_PS),
//                SimTime(1333952500, SIMTIME_PS), SimTime(602293500, SIMTIME_PS),
//                SimTime(966248500, SIMTIME_PS), SimTime(1338686000, SIMTIME_PS),
//                SimTime(587513500, SIMTIME_PS), SimTime(999311000, SIMTIME_PS),
//                SimTime(1333727000, SIMTIME_PS), SimTime(601031500, SIMTIME_PS),
//                SimTime(968510000, SIMTIME_PS), SimTime(1340979000, SIMTIME_PS),
//                SimTime(586090500, SIMTIME_PS), SimTime(987556500, SIMTIME_PS),
//                SimTime(1495193000, SIMTIME_PS), SimTime(605150500, SIMTIME_PS),
//                SimTime(972399500, SIMTIME_PS), SimTime(1354451000, SIMTIME_PS),
//                SimTime(588239500, SIMTIME_PS), SimTime(993769500, SIMTIME_PS),
//                SimTime(1332182000, SIMTIME_PS), SimTime(611251500, SIMTIME_PS),
//                SimTime(984363000, SIMTIME_PS), SimTime(1368996000, SIMTIME_PS),
//                SimTime(602952000, SIMTIME_PS), SimTime(996895000, SIMTIME_PS),
//                SimTime(1357962500, SIMTIME_PS), SimTime(615189500, SIMTIME_PS),
//                SimTime(989255000, SIMTIME_PS), SimTime(1370287500, SIMTIME_PS),
//                SimTime(601601500, SIMTIME_PS), SimTime(999744500, SIMTIME_PS),
//                SimTime(411394500, SIMTIME_PS) };
//        backward_pass_time =
//                vector<simtime_t> { SimTime(416228000, SIMTIME_PS), SimTime(
//                        229544768000, SIMTIME_PS), SimTime(6106568500,
//                        SIMTIME_PS), SimTime(8326861000, SIMTIME_PS), SimTime(
//                        14241184000, SIMTIME_PS), SimTime(6104166500,
//                        SIMTIME_PS), SimTime(8338906500, SIMTIME_PS), SimTime(
//                        14249240000, SIMTIME_PS), SimTime(6113811000,
//                        SIMTIME_PS), SimTime(8326589000, SIMTIME_PS), SimTime(
//                        14238364500, SIMTIME_PS), SimTime(6109754000,
//                        SIMTIME_PS), SimTime(8344041000, SIMTIME_PS), SimTime(
//                        14020027500, SIMTIME_PS), SimTime(2887953000,
//                        SIMTIME_PS), SimTime(7535392000, SIMTIME_PS), SimTime(
//                        15202323000, SIMTIME_PS), SimTime(10084485000,
//                        SIMTIME_PS), SimTime(6085473500, SIMTIME_PS), SimTime(
//                        15277926000, SIMTIME_PS), SimTime(10172296000,
//                        SIMTIME_PS), SimTime(7242516000, SIMTIME_PS), SimTime(
//                        15409178500, SIMTIME_PS), SimTime(8956063000,
//                        SIMTIME_PS), SimTime(7463055500, SIMTIME_PS), SimTime(
//                        15079349000, SIMTIME_PS), SimTime(9197268000,
//                        SIMTIME_PS), SimTime(7383737500, SIMTIME_PS), SimTime(
//                        1604860500, SIMTIME_PS), SimTime(810691500, SIMTIME_PS),
//                        SimTime(512126500, SIMTIME_PS), SimTime(1188866500,
//                                SIMTIME_PS), SimTime(809201000, SIMTIME_PS),
//                        SimTime(512058000, SIMTIME_PS), SimTime(1190091000,
//                                SIMTIME_PS), SimTime(825784000, SIMTIME_PS),
//                        SimTime(534169000, SIMTIME_PS), SimTime(4128649500,
//                                SIMTIME_PS) };
//        weight_update_time = vector<simtime_t> { SimTime(17613563287,
//                SIMTIME_PS), SimTime(5029973950, SIMTIME_PS), SimTime(
//                4139141215, SIMTIME_PS), SimTime(5321917838, SIMTIME_PS),
//                SimTime(4733414348, SIMTIME_PS), SimTime(4139141215,
//                        SIMTIME_PS), SimTime(5321917838, SIMTIME_PS), SimTime(
//                        4733414348, SIMTIME_PS), SimTime(4139141215,
//                        SIMTIME_PS), SimTime(5321917838, SIMTIME_PS), SimTime(
//                        4733414348, SIMTIME_PS), SimTime(4139141215,
//                        SIMTIME_PS), SimTime(5321917838, SIMTIME_PS), SimTime(
//                        4733414348, SIMTIME_PS), SimTime(4139141215,
//                        SIMTIME_PS), SimTime(5321917838, SIMTIME_PS), SimTime(
//                        4733414348, SIMTIME_PS), SimTime(4139141215,
//                        SIMTIME_PS), SimTime(5321917838, SIMTIME_PS), SimTime(
//                        4733414348, SIMTIME_PS), SimTime(4139141215,
//                        SIMTIME_PS), SimTime(5321917838, SIMTIME_PS), SimTime(
//                        4733414348, SIMTIME_PS), SimTime(4139141215,
//                        SIMTIME_PS), SimTime(5321917838, SIMTIME_PS), SimTime(
//                        4733414348, SIMTIME_PS), SimTime(4139141215,
//                        SIMTIME_PS), SimTime(5321917838, SIMTIME_PS), SimTime(
//                        4733414348, SIMTIME_PS), SimTime(4139141215,
//                        SIMTIME_PS), SimTime(5321917838, SIMTIME_PS), SimTime(
//                        4733414348, SIMTIME_PS), SimTime(4139141215,
//                        SIMTIME_PS), SimTime(5321917838, SIMTIME_PS), SimTime(
//                        4733414348, SIMTIME_PS), SimTime(4139141215,
//                        SIMTIME_PS), SimTime(5321917838, SIMTIME_PS), SimTime(
//                        593697295, SIMTIME_PS) };
//    } else if (m == "test") {
//        model = vector<uint64_t> { uint64_t(worker->par("test_tensor_size")) };
//        forward_pass_time = vector<simtime_t> { 0 };
//        backward_pass_time = vector<simtime_t> { 0 };
//        weight_update_time = vector<simtime_t> { 0 };
//    } else { // resnet50
//        model = vector<uint64_t> { 405824, 6755584, 7417344, 7875584, 3102696 };
//        forward_pass_time = vector<simtime_t> { SimTime(36421561000,
//                SIMTIME_PS), SimTime(45085059500, SIMTIME_PS), SimTime(
//                13925746500, SIMTIME_PS), SimTime(6352587000, SIMTIME_PS),
//                SimTime(2089498000, SIMTIME_PS) };
//        backward_pass_time = vector<simtime_t> { SimTime(77064011000,
//                SIMTIME_PS), SimTime(83514474000, SIMTIME_PS), SimTime(
//                28129353500, SIMTIME_PS), SimTime(11618227000, SIMTIME_PS),
//                SimTime(3549431000, SIMTIME_PS) };
//        weight_update_time =
//                vector<simtime_t> { SimTime(20061435, SIMTIME_PS), SimTime(
//                        333954398, SIMTIME_PS), SimTime(366667730, SIMTIME_PS),
//                        SimTime(389320288, SIMTIME_PS), SimTime(153378150,
//                                SIMTIME_PS) };
//    }
//    // all time numbers are picoseconds
//}
