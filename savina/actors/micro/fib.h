#include <memory>
#include <test/harness.h>
#include <cpp/when.h>
#include "util/bench.h"
#include <random>

namespace actor_benchmark {

namespace fib {

using namespace std;

enum class None {};

struct Fibonacci {
  cown_ptr<Fibonacci> parent;
  uint64_t responses;
  uint64_t result;

  Fibonacci(): responses(0), result(0) {}

  Fibonacci(cown_ptr<Fibonacci> parent): parent(move(parent)), responses(0), result(0) {}

  static void root(int64_t n) { Fibonacci::compute(make_cown<Fibonacci>(), n); }

  static void request(cown_ptr<Fibonacci>& parent, int64_t n) {
    Fibonacci::compute(make_cown<Fibonacci>(parent), n);
  }

  static void response(cown_ptr<Fibonacci>& self, uint64_t n) {
    when(self) << [n](acquired_cown<Fibonacci> self)  mutable {
      self->result += n;
      self->responses++;

      if (self->responses == 2)
        self->propagate();
    };
  }

  static void compute(cown_ptr<Fibonacci>&& self, int64_t n) {
    when(self) << [tag=self, n](acquired_cown<Fibonacci> self) mutable {
      if (n <= 2) {
        self->result = 1;
        self->propagate();
      } else {
        Fibonacci::request(tag, n-1);
        Fibonacci::request(tag, n-2);
      }
    };
  }

  void propagate() {
    if (parent)
      Fibonacci::response(parent, result);
  }

};

};

struct Fib: public AsyncBenchmark {
  uint64_t index;

  Fib(uint64_t index): index(index) {}

  void run() { fib::Fibonacci::root(index); }

  std::string name() { return "Fib"; }

};


};