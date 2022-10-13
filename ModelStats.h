#ifndef MODELSTATS_H_
#define MODELSTATS_H_
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>
#include <omnetpp.h>
using namespace omnetpp;

constexpr short alexnet = 0;
constexpr short bert = 1;
constexpr short googlenet = 2;
constexpr short inception = 3;
constexpr short resnet101 = 4;
constexpr short resnet152 = 5;
constexpr short resnet50 = 6;
constexpr short vgg11 = 7;
constexpr short vgg16 = 8;
constexpr short vgg19 = 9;

constexpr size_t num_models = 10 + 1; // +1 for custom model

extern size_t n_layers[num_models];

extern std::vector<int64_t> model_sizes[num_models];

extern std::vector<simtime_t> fp_times[num_models];

extern std::vector<simtime_t> bp_times[num_models];

extern std::vector<simtime_t> wu_times[num_models];

extern std::vector<const char*> fp_times_raw;

extern std::vector<const char*> bp_times_raw;

extern std::vector<const char*> wu_times_raw;

extern simtime_t all_fps_and_last_bp_times[num_models];
extern simtime_t all_fps_and_bps_times[num_models];
extern std::vector<simtime_t> min_wait_times[num_models];
extern std::vector<simtime_t> min_wait_times_wu[num_models];
extern simtime_t min_wait_time_sums[num_models];
extern simtime_t min_wait_time_sums_wu[num_models];

simtime_t all_fps_and_last_bp(short m);
simtime_t all_fps_and_bps(short m);
std::vector<simtime_t> min_wait_time(short m, bool wu_as_busy = false,
        uint64_t gbps = 100);

#endif /* MODELSTATS_H_ */
