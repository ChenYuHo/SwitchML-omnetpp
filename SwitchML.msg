struct TensorKey {
	uint64_t layer;
	uint64_t job_id;
}

cplusplus(h) {{
	namespace std {
        template<> struct hash<TensorKey> {
            std::size_t operator()(const TensorKey &k) const {
                size_t lhs = std::hash<uint64_t>()(k.layer);
                size_t rhs = std::hash<uint64_t>()(k.job_id);
                lhs ^= rhs + 0x9e3779b9 + (lhs << 6) + (lhs >> 2);
                return lhs;
            }
        };
    }
    bool operator==(const TensorKey&, const TensorKey&);
    bool operator<(const TensorKey&, const TensorKey&);
}}

cplusplus(cc) {{
    bool operator==(const TensorKey& left, const TensorKey& right) {
        return left.layer == right.layer && left.job_id == right.job_id;
    }

    bool operator<(const TensorKey& left, const TensorKey& right) {
        // return true when right has smaller layer (higher priority)
        return left.layer > right.layer;
    }
}}

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
    TensorKey tensor_key;
    uint32_t n_workers;
    uint64_t num_pkts_expected;
    uint64_t grad_size;
    bool upward;
}

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
    int training_process_id;
    int worker_id;
    bool completed;
    short model;
    uint64_t size;
    uint64_t rank;
    TensorKey tensor_key;
    uint64_t num_workers_allocated;
    uint64_t num_chunks = 1;
    uint64_t chunk_id = 0;
    simtime_t start;
};

message Job
{
    simtime_t submit_time = -1;
    simtime_t start_time = -1;
    simtime_t finish_time = -1;
    short model = 0;
    uint64_t job_id = 0;
    int worker_id = 0;
    uint64_t gpu = 8;
    uint64_t rank = 0;
    uint32_t num_workers_allocated = 2;
    uint64_t iters = 2;
};

message Instruction
{
	uint64_t layer = 0;
	uint64_t iter = 0;
}