#include <memory>
#include <test/harness.h>
#include <cpp/when.h>
#include "util/bench.h"

namespace ActorBenchmark {

namespace {

using verona::cpp::make_cown;
using verona::cpp::cown_ptr;
using verona::cpp::acquired_cown;

struct Arbitator;

struct Philosopher {
  size_t id;
  uint64_t local;
  uint64_t rounds;
  cown_ptr<Arbitator> arbitator;

  Philosopher(size_t id, uint64_t rounds, cown_ptr<Arbitator> arbitator): id(id), local(0), rounds(rounds), arbitator(arbitator) {}

  static void start(cown_ptr<Philosopher>);
  static void denied(cown_ptr<Philosopher>);
  static void eat(cown_ptr<Philosopher>);
};

struct Arbitator {
  std::vector<bool> forks;
  uint64_t philosophers;
  uint64_t done_eating;

  Arbitator(uint64_t philosophers): philosophers(philosophers), done_eating(philosophers) {
    for (uint64_t i = 0; i < philosophers; ++i)
      forks.push_back(false);
  }

  static void hungry(cown_ptr<Arbitator> self, cown_ptr<Philosopher> philosopher, uint64_t id) {
    when(self) << [philosopher, id](acquired_cown<Arbitator> self) {
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

  static void done(cown_ptr<Arbitator> self, uint64_t id) {
    when(self) << [id](acquired_cown<Arbitator> self) {
      self->forks[id] = false;
      self->forks[(id + 1) % self->philosophers] = false;
    };
  }

  static void finished(cown_ptr<Arbitator> self) {
    when(self) << [](acquired_cown<Arbitator> self) {
      if (--(self->done_eating) == 0) {
        return;
      }
    };
  }
};

void Philosopher::start(cown_ptr<Philosopher> self) {
  when(self) << [tag=self](acquired_cown<Philosopher> self) {
    Arbitator::hungry(self->arbitator, tag, self->id);
  };
}

void Philosopher::denied(cown_ptr<Philosopher> self) {
  when(self) << [tag=self](acquired_cown<Philosopher> self) {
    self->local++;
    Arbitator::hungry(self->arbitator, tag, self->id);
  };
}

void Philosopher::eat(cown_ptr<Philosopher> self) {
  when(self) << [tag=self](acquired_cown<Philosopher> self) {
    Arbitator::done(self->arbitator, self->id);

    if (--self->rounds >= 1) {
      Philosopher::start(tag);
    } else {
      Arbitator::finished(self->arbitator);
    }
  };
}

struct DiningPhilosophers: public AsyncBenchmark {
  uint64_t philosophers;
  uint64_t rounds;
  uint64_t channels;

  DiningPhilosophers(uint64_t philosophers, uint64_t rounds, uint64_t channels): philosophers(philosophers), rounds(rounds), channels(channels) {}

  void run() {
    cown_ptr<Arbitator> arbitator = make_cown<Arbitator>(philosophers);
    std::vector<cown_ptr<Philosopher>> actors;

    for (uint64_t i = 0; i < philosophers; ++i) {
      actors.push_back(make_cown<Philosopher>(i, rounds, arbitator));
    }

    for (const auto& actor: actors) {
      Philosopher::start(actor);
    }
  }

  std::string name() { return "Dining Philosophers"; }
};

};

};