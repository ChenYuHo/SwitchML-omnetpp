//
// This file is part of an OMNeT++/OMNEST simulation example.
//
// Copyright (C) 1992-2015 Andras Varga
//
// This file is distributed WITHOUT ANY WARRANTY. See the file
// `license' for details on this and other legal matters.
//


enum SwitchMLPacketType
{
    SWITCHML_CONN_REQ = 0;
    SWITCHML_CONN_ACK = 1;
    SWITCHML_DISC_REQ = 2;
    SWITCHML_DISC_ACK = 3;
    SWITCHML_DATA = 4;
};

packet SwitchMLPacket
{
    int srcAddress;
    int destAddress;
    int serverProcId;
};

message LayerAck
{
    uint64_t layer;
};

message Setup
{
    int id;
};

message AllreduceRequest
{
    uint64_t size;
    uint64_t rank;
};

message JobInfo
{
    uint64_t id = 0;
    uint64_t gpu = 8;
    uint64_t rank = 0;
    uint32_t num_workers_allocated = 2;
    uint64_t iters = 2;
    uint64_t fp_times[] = 1;
    uint64_t bp_times[] = 1;
    uint64_t grad_sizes[] = 100;
};