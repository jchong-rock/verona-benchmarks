#include <cpp/when.h>
#include "util/bench.h"
#include "util/random.h"
#include <cmath>

namespace boc_benchmark {

namespace fjthrput {

// I'm not sure there is anything to BoC-ify here, nothing really happens,
// no values are computed and accumulated.

using namespace std;

struct FjthrMaster;

struct Throughput {
  cown_ptr<FjthrMaster> master;

  Throughput(cown_ptr<FjthrMaster> master): master(master) {}

  static void compute(cown_ptr<Throughput>);
};

struct FjthrMaster {
  uint64_t total;

  FjthrMaster(uint64_t messages, uint64_t actors): total(messages * actors) {}

  static void make(uint64_t messages, uint64_t actors, uint64_t channels, bool priorities) {
    cown_ptr<FjthrMaster> master = make_cown<FjthrMaster>(messages, actors);
    vector<cown_ptr<Throughput>> throughputs;

    for (uint64_t i = 0; i < actors; ++i) {
      throughputs.emplace_back(make_cown<Throughput>(master));
    }

    for (uint64_t j = 0; j < messages; ++j) {
      for(const cown_ptr<Throughput>& k: throughputs) {
        Throughput::compute(k);
      }
    }

    for(const cown_ptr<Throughput>& k: throughputs) {
      when(master, k) << [](acquired_cown<FjthrMaster> master, acquired_cown<Throughput> k){
        if (--master->total) { /* done */ }
      };
    }
    /*
    when(throughputs) {
      combine
    }
    */
  }
};

void Throughput::compute(cown_ptr<Throughput> throughput) {
  when(throughput) << [](acquired_cown<Throughput> throughput) {
    double n = sin(37.2);
    double r = n * n;
  };
}

};

struct Fjthrput: public BocBenchmark {
  uint64_t messages;
  uint64_t actors;
  uint64_t channels;
  bool priorities;

  Fjthrput(uint64_t messages, uint64_t actors, uint64_t channels, bool priorities)
    : messages(messages), actors(actors), channels(channels), priorities(priorities) {}

  void run() { fjthrput::FjthrMaster::make(messages, actors, channels, priorities); }

  std::string name() { return "Fork-Join Throughput"; }
};

};