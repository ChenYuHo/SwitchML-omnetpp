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
    uint64_t job_id;
    bool completed;
};

message HierarchyQuery
{
    int path[];
    int from_id;
    int num_gpus;
}

message Setup
{
   uint64_t job_id;
   int ids[];
   bool top_level;
};

message CollectiveOperationRequest
{
    int training_process_id;
    int worker_id;
    short model;
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
    int64_t submit_time = -1;
    int64_t start_time = -1;
    int64_t finish_time = -1;
    short model = 0;
    uint64_t job_id = 0;
    int worker_id = 0;
    uint64_t gpu = 8;
    uint64_t rank = 0;
    uint32_t num_workers_allocated = 2;
    uint64_t iters = 2;
};