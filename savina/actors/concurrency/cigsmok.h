#include <memory>
#include <test/harness.h>
#include <cpp/when.h>
#include "util/bench.h"

namespace ActorBenchmark {

using verona::cpp::make_cown;
using verona::cpp::cown_ptr;
using verona::cpp::acquired_cown;

struct Arbiter;

struct Smoker {
  cown_ptr<Arbiter> arbiter;
  SimpleRand random;

  Smoker(cown_ptr<Arbiter> arbiter): arbiter(arbiter), random(12345) {};

  static void smoke(cown_ptr<Smoker>, uint64_t);
};

struct Arbiter {
  SimpleRand random;
  std::vector<cown_ptr<Smoker>> smokers;
  uint64_t rounds;

  Arbiter(uint64_t rounds): random(12345), rounds(rounds) {}

  static void add_smokers(cown_ptr<Arbiter> self, uint64_t num_smokers) {
    when(self) << [tag=self, num_smokers](acquired_cown<Arbiter> self) {
      for (uint64_t i = 0; i < num_smokers; ++i)
        self->smokers.push_back(make_cown<Smoker>(tag));
    };
  }

  static void notify_smoker(cown_ptr<Arbiter> self) {
    when(self) << [](acquired_cown<Arbiter> self) {
      uint64_t index = self->random.nextInt() % self->smokers.size();
      Smoker::smoke(self->smokers[index], self->random.nextInt(1000) + 10);
    };
  }

  static void started(cown_ptr<Arbiter> self) {
    when(self) << [tag=self](acquired_cown<Arbiter> self) {
      if (--self->rounds > 0) {
        Arbiter::notify_smoker(tag);
      } else {
        return;
      }
    };
  }
};

void Smoker::smoke(cown_ptr<Smoker> self, uint64_t period) {
  when(self) << [period](acquired_cown<Smoker> self) {
    Arbiter::started(self->arbiter);

    // TODO: I don't think this is doing anything?
    uint64_t test = 0;

    for (uint64_t i = 0; i < period; ++i) {
      test++;
    }
  };
}


struct Cigsmok: public AsyncBenchmark {
  uint64_t rounds;
  uint64_t smokers;

  Cigsmok(uint64_t rounds, uint64_t smokers): rounds(rounds), smokers(smokers) {};

  void run() {
    cown_ptr<Arbiter> arbiter = make_cown<Arbiter>(rounds);
    Arbiter::add_smokers(arbiter, smokers);
    Arbiter::notify_smoker(arbiter);
  }

  std::string name() { return "Cigarette Smokers"; }
};

};