#include <memory>
#include <test/harness.h>
#include <cpp/when.h>
#include "util/bench.h"

namespace boc_benchmark {

namespace philosopher {

using namespace verona::cpp;
using namespace std;

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

};

// Even though this is the naive way - i'm still seeing speed up
struct DiningPhilosophers: public AsyncBenchmark {
  uint64_t philosophers;
  uint64_t rounds;

  DiningPhilosophers(uint64_t philosophers, uint64_t rounds): philosophers(philosophers), rounds(rounds) {}

  void run() {
    using namespace philosopher;

    cown_ptr<Table> table = make_cown<Table>(philosophers);
    std::vector<cown_ptr<Philosopher>> phils; // should we be allow to make actors iso?

    cown_ptr<Fork> first = make_cown<Fork>();
    cown_ptr<Fork> prev = first;
    for (uint64_t i = 0; i < philosophers - 1; ++i) {
      cown_ptr<Fork> next = make_cown<Fork>();
      phils.emplace_back(make_cown<Philosopher>(i, rounds, prev, next, table));
      prev = next;
    }
    phils.emplace_back(make_cown<Philosopher>(philosophers - 1, rounds, prev, first, table));

    while(!phils.empty()) {
      Philosopher::eat(phils.back());
      phils.pop_back();
    }
  }

  std::string name() { return "Dining Philosophers"; }
};

};