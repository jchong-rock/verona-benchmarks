#include <cpp/when.h>
#include "util/bench.h"
#include "util/random.h"
#include <variant>

namespace actor_benchmark {

namespace threadring {

using namespace std;

enum class None {};

struct RingActor {
  variant<cown_ptr<RingActor>, None> _next;

  RingActor(): _next(None()) {}

  RingActor(cown_ptr<RingActor> next): _next(next) {}

  static void next(cown_ptr<RingActor> self, cown_ptr<RingActor> neighbor) {
    when(self) << [neighbor](acquired_cown<RingActor> self) {
      self->_next = neighbor;
    };
  }

  static void pass(cown_ptr<RingActor> self, uint64_t left) {
    when(self) << [left](acquired_cown<RingActor> self) {
      if (left > 0) {
        visit(overloaded {
          [&](None&) { /* this can't actually happen because it's a ring */ },
          [&](cown_ptr<RingActor>& n) { RingActor::pass(n, left - 1); }
        }, self->_next);
      } else {
        /* done */
      }
    };
  }
};

};

struct ThreadRing: public AsyncBenchmark {
  uint64_t actors;
  uint64_t pass;

  ThreadRing(uint64_t actors, uint64_t pass): actors(actors), pass(pass) {}

  void run() {
    using namespace threadring;

    auto first = make_cown<RingActor>();
    auto next = first;

    for (uint64_t k = 0; k < actors - 1; ++k) {
      auto current = make_cown<RingActor>(next);
      next = current;
    }

    RingActor::next(first, next);

    if (pass > 0) {
      RingActor::pass(first, pass);
    }
  }

  std::string name() { return "Thread Ring"; }
};

};

