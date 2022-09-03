//
// This file is part of an OMNeT++/OMNEST simulation example.
//
// Copyright (C) 1992-2015 Andras Varga
//
// This file is distributed WITHOUT ANY WARRANTY. See the file
// `license' for details on this and other legal matters.
//


enum SwitchMLPacketType//        model = vector<uint64_t> { 405824, 6755584, 7417344, 7875584, 3102696 };

{
    SWITCHML_CONN_REQ = 0;
    SWITCHML_CONN_ACK = 1;
    SWITCHML_DISC_REQ = 2;
    SWITCHML_DISC_ACK = 3;
    SWITCHML_DATA = 4;
};

//enum Model
//{
//    alexnet = 0;
//    vgg11 = 1;
//    vgg16 = 2;
//    vgg19 = 3;
//    inception = 4;
//    googlenet = 5;
//    resnet50 = 6;
//    resnet101 = 7;
//    resnet152 = 8;
//    bert = 9;
//}

packet SwitchMLPacket
{
    byteLength = 9000;
    int from_id;
    uint32_t slot;
    uint32_t ver;
    uint32_t offset;
    uint32_t tensor_key;
    uint32_t n_workers;
    uint64_t layer;
    uint64_t job_id;
    uint64_t num_pkts_expected;
    uint64_t grad_size;
    uint64_t num_chunks;
    uint64_t chunk_id;
    short model;
    bool upward;
}

message LayerAck
{
    uint64_t layer;
    simtime_t weight_update_time;
    uint64_t job_id;
    bool completed;
};

message HierarchyQuery
{
    int path[];
    int from_id;
    int num_gpus;
    cModule* modules[];
}

message Setup
{
   uint64_t job_id;
   int ids[];
   bool top_level;
};

message CollectiveOperationRequest
{
    int allreducer_id;
    int training_process_id;
    int worker_id;
    uint64_t size;
    uint64_t rank;
    uint64_t layer;
    uint64_t tensor_key;
    uint64_t job_id;
    uint64_t num_workers_allocated;
    uint64_t num_chunks = 1;
    uint64_t chunk_id = 0;
};

message Job
{
    simtime_t submit_time = 0;
    simtime_t start_time = 0;
    simtime_t finish_time = 0;
    short model = 0;
    uint64_t job_id = 0;
    int worker_id = 0;
    uint64_t gpu = 8;
    uint64_t rank = 0;
    uint32_t num_workers_allocated = 2;
    uint64_t iters = 2;
};