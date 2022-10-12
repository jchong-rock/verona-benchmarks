#include <memory>
#include <test/harness.h>
#include <cpp/when.h>
#include "util/bench.h"

namespace actor_benchmark {

namespace philosopher {

using namespace verona::cpp;
using namespace std;

struct Arbitator;

struct Philosopher {
  size_t id;
  uint64_t local;
  uint64_t rounds;
  cown_ptr<Arbitator> arbitator;

  Philosopher(size_t id, uint64_t rounds, cown_ptr<Arbitator> arbitator): id(id), local(0), rounds(rounds), arbitator(move(arbitator)) {}

  static void start(const cown_ptr<Philosopher>&);
  static void denied(const cown_ptr<Philosopher>&);
  static void eat(const cown_ptr<Philosopher>&);
};

struct Arbitator {
  std::vector<bool> forks;
  uint64_t philosophers;
  uint64_t done_eating;

  Arbitator(uint64_t philosophers): philosophers(philosophers), done_eating(philosophers) {
    for (uint64_t i = 0; i < philosophers; ++i)
      forks.push_back(false);
  }

  static void hungry(const cown_ptr<Arbitator>& self, cown_ptr<Philosopher> philosopher, uint64_t id) {
    when(self) << [philosopher=move(philosopher), id](acquired_cown<Arbitator> self)  mutable {
      uint64_t right_index = ((id + 1) % self->philosophers);
      if (self->forks[id] || self->forks[right_index]) {
        Philosopher::denied(philosopher);
      } else {
        self->forks[id] = true;
        self->forks[right_index] = true;
        Philosopher::eat(philosopher);
      }
    };
  }

  static void done(const cown_ptr<Arbitator>& self, uint64_t id) {
    when(self) << [id](acquired_cown<Arbitator> self)  mutable {
      self->forks[id] = false;
      self->forks[(id + 1) % self->philosophers] = false;
    };
  }

  static void finished(const cown_ptr<Arbitator>& self) {
    when(self) << [](acquired_cown<Arbitator> self)  mutable {
      if (--(self->done_eating) == 0) {
        return;
      }
    };
  }
};

void Philosopher::start(const cown_ptr<Philosopher>& self) {
  when(self) << [tag=self](acquired_cown<Philosopher> self)  mutable {
    Arbitator::hungry(self->arbitator, move(tag), self->id);
  };
}

void Philosopher::denied(const cown_ptr<Philosopher>& self) {
  when(self) << [tag=self](acquired_cown<Philosopher> self)  mutable {
    self->local++;
    Arbitator::hungry(self->arbitator, move(tag), self->id);
  };
}

void Philosopher::eat(const cown_ptr<Philosopher>& self) {
  when(self) << [tag=self](acquired_cown<Philosopher> self)  mutable {
    Arbitator::done(self->arbitator, self->id);

    if (--self->rounds >= 1) {
      Philosopher::start(tag);
    } else {
      Arbitator::finished(self->arbitator);
    }
  };
}

};

struct DiningPhilosophers: public AsyncBenchmark {
  uint64_t philosophers;
  uint64_t rounds;
  uint64_t channels;

  DiningPhilosophers(uint64_t philosophers, uint64_t rounds, uint64_t channels): philosophers(philosophers), rounds(rounds), channels(channels) {}

  void run() {
    using namespace philosopher;

    cown_ptr<Arbitator> arbitator = make_cown<Arbitator>(philosophers);
    std::vector<cown_ptr<Philosopher>> actors;

    for (uint64_t i = 0; i < philosophers; ++i) {
      actors.emplace_back(make_cown<Philosopher>(i, rounds, arbitator));
    }

    for (const auto& actor: actors) {
      Philosopher::start(actor);
    }
  }

  std::string name() { return "Dining Philosophers"; }
};

};