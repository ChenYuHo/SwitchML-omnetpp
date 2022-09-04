#include "Sincronia.h"
Define_Module(Sincronia);

void Sincronia::initialize() {
//    scheduleAt(simTime(), new cMessage);

}

/*    int allreducer_id;
 int training_process_id;
 int worker_id;
 uint64_t size;
 uint64_t rank;
 uint64_t layer;
 uint64_t tensor_key;
 uint64_t job_id;
 uint64_t num_workers_allocated;
 uint64_t num_chunks = 1;
 uint64_t chunk_id = 0;*/

void Sincronia::handleMessage(cMessage *msg) {
    switch (msg->getKind()) {
    case 0: {
        // AllreduceRequest from TrainingProcess
        auto request = check_and_cast<CollectiveOperationRequest*>(msg);
        auto &requests = queue[request->getTensor_key()];
        requests.push_back(request);
        if (requests.size() == request->getNum_workers_allocated()) {
            for (auto req : requests) {
                auto reducer = this->getSimulation()->getModule(req->getWorker_id());
                this->sendDirect(req, reducer, "directin");
            }
            queue.erase(request->getTensor_key());
        }
        break;
    }
    case 2: {
        // LayerAck from Worker, meaning allreduce is done
        delete msg;
        break;
    }
    default:
        delete msg;
        EV_FATAL << "got unexpected message" << endl;
        break;
    }

}

