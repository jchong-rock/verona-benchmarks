#include <memory>
#include <debug/harness.h>
#include <cpp/when.h>
#include "util/bench.h"
#include <random>

namespace actor_benchmark {

struct Nop: public ActorBenchmark {
  Nop() {}

  void run() { }

  std::string name() { return "Nop"; }

};

}