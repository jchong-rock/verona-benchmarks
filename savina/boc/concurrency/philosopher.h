#include <memory>
#include <test/harness.h>
#include <cpp/when.h>
#include "util/bench.h"

namespace BOCBenchmark {

namespace {

using namespace verona::cpp;
using namespace std;

#if 0

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

  DiningPhilosophers(uint64_t philosophers, uint64_t rounds): philosophers(philosophers), rounds(rounds) {}

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

#else
struct Table {
  uint64_t done_eating;

  Table(uint64_t philosophers): done_eating(philosophers) { }

  static void finished(cown_ptr<Table> self) {
    when(self) << [](acquired_cown<Table> self) {
      if (--(self->done_eating) == 0) {
        return;
      }
    };
  }
};

struct Fork {
  Fork(){}
};

struct Philosopher {
  size_t id;
  // uint64_t local; - we don't need local as we cannot be denied to try again
  uint64_t rounds;
  cown_ptr<Fork> left;
  cown_ptr<Fork> right;
  cown_ptr<Table> table;

  Philosopher(size_t id, uint64_t rounds, cown_ptr<Fork> left, cown_ptr<Fork> right, cown_ptr<Table> table):
    id(id), rounds(rounds), left(left), right(right), table(table) {}

  static void eat(cown_ptr<Philosopher> phil) {
    when(phil) << [tag=phil] (acquired_cown<Philosopher> phil) { // sneakily makes things faster by breaking order?
      if (--phil->rounds >= 1) {
        when(phil->left, phil->right) << [](acquired_cown<Fork> left, acquired_cown<Fork> right) {}; // should phil be here?
        eat(tag);
      } else {
        Table::finished(phil->table);
      }
    };
  }
};

// Even though this is the naive way - i'm still seeing speed up
struct DiningPhilosophers: public AsyncBenchmark {
  uint64_t philosophers;
  uint64_t rounds;

  DiningPhilosophers(uint64_t philosophers, uint64_t rounds): philosophers(philosophers), rounds(rounds) {}

  void run() {
    cown_ptr<Table> table = make_cown<Table>(philosophers);
    std::vector<cown_ptr<Philosopher>> phils; // should we be allow to make actors iso?

    cown_ptr<Fork> first = make_cown<Fork>();
    cown_ptr<Fork> prev = first;
    for (uint64_t i = 0; i < philosophers - 1; ++i) {
      cown_ptr<Fork> next = make_cown<Fork>();
      phils.push_back(make_cown<Philosopher>(i, rounds, prev, next, table));
      prev = next;
    }
    phils.push_back(make_cown<Philosopher>(philosophers - 1, rounds, prev, first, table));

    while(!phils.empty()) {
      Philosopher::eat(phils.back());
      phils.pop_back();
    }
  }

  std::string name() { return "Dining Philosophers"; }
};
#endif

};

};