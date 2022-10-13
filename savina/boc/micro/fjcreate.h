#include <cpp/when.h>
#include "util/bench.h"
#include "util/random.h"
#include <cmath>

namespace boc_benchmark {

namespace fjcreate {

using namespace std;

struct Token {};

struct ForkJoin {
  static cown_ptr<ForkJoin> make(Token token) {
    auto worker = make_cown<ForkJoin>();
    when(worker) << [](acquired_cown<ForkJoin>) { // this isn't forking, it's all done in one thread
      double n = sin(double(37.2));
      double r = n * n;
    };
    return worker;
  }
};

struct ForkJoinMaster {
  uint64_t workers;

  ForkJoinMaster(uint64_t workers): workers(workers) {}

  static void make(uint64_t workers) {
    cown_ptr<ForkJoinMaster> master = make_cown<ForkJoinMaster>(workers);
    vector<cown_ptr<ForkJoin>> fjs;

    for (uint64_t i = 0; i < workers; ++i) {
      fjs.push_back(ForkJoin::make(Token{}));
    }

    for (const auto& worker: fjs) {
      when(master, worker) << [](acquired_cown<ForkJoinMaster> master, acquired_cown<ForkJoin> worker) {
        master->workers--;
      };
    }
  }
};

};

struct Fjcreate: public AsyncBenchmark {
  uint64_t workers;

  Fjcreate(uint64_t workers): workers(workers) {}

  void run() { fjcreate::ForkJoinMaster::make(workers); }

  std::string name() { return "Fork-Join Create"; }
};

};