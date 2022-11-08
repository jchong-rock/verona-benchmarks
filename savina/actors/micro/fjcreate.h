#include <cpp/when.h>
#include "util/bench.h"
#include "util/random.h"
#include <cmath>

namespace actor_benchmark {

namespace fjcreate {

using namespace std;

struct Token {};

struct ForkJoinMaster {
  uint64_t workers;

  ForkJoinMaster(uint64_t workers): workers(workers) {}
  static void make(uint64_t workers);
  static void done(const cown_ptr<ForkJoinMaster>&);
};

namespace ForkJoin {
  static void make(const cown_ptr<ForkJoinMaster>& master, Token token) {
    when() << [master]() mutable {
      double n = sin(double(37.2));
      double r = n * n;
      ForkJoinMaster::done(master);
    };
  }
};

void ForkJoinMaster::make(uint64_t workers) {
  cown_ptr<ForkJoinMaster> master = make_cown<ForkJoinMaster>(workers);

  for (uint64_t i = 0; i < workers; ++i) {
    ForkJoin::make(master, Token{});
  }
}

void ForkJoinMaster::done(const cown_ptr<ForkJoinMaster>& self) {
  when(self) << [](acquired_cown<ForkJoinMaster> self)  mutable{
    self->workers--;
  };
}

};

struct Fjcreate: public ActorBenchmark {
  uint64_t workers;

  Fjcreate(uint64_t workers): workers(workers) {}

  void run() { fjcreate::ForkJoinMaster::make(workers); }

  std::string name() { return "Fork-Join Create"; }
};

};