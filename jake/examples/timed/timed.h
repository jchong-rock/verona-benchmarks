#include "util/bench.h"
#include "util/random.h"
#include "leader_ring_timed.h"
#include "leader_ring_boc_timed.h"
#include <chrono>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

namespace jake_benchmark {

struct TimedBench: public BocBenchmark {
    uint64_t servers;
    uint64_t repetitions;
    TimedBench(uint64_t servers, uint64_t repetitions): servers(servers), repetitions(repetitions) {}
    void run() {
        std::vector<uint64_t> ids;
        std::vector<uint64_t> starts;
        for (uint64_t i = 0; i < repetitions; i++) {
            ids = gen_x_unique_randoms<uint64_t>(servers, std::max(servers*2, 65535ul));
            starts = gen_x_unique_randoms<uint64_t>(1, servers-2);

            for (uint64_t i = 0; i < repetitions; i++) {
                pid_t p = fork();
                if (p == 0) {
                    using namespace leader_ring_timed;
                    LeaderRingTimed::make(servers, ids, starts[0]);
                }
                else {
                    waitpid(p, NULL, 0);
                }
            }
            for (uint64_t i = 0; i < repetitions; i++) {
                pid_t p = fork();
                if (p == 0) {
                    using namespace leader_ring_boc_timed;
                    LeaderRingBoCTimed::make(servers, ids, starts[0]);
                }
                else {
                    waitpid(p, NULL, 0);
                }
            }
        }
    }
};
};