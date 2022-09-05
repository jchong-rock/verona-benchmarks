#include <memory>
#include <test/harness.h>
#include <cpp/when.h>
#include "util/bench.h"
#include <random>
#include <variant>

namespace ActorBenchmark {

namespace {

using namespace std;

struct None {};

struct Fibonacci {
  variant<cown_ptr<Fibonacci>, None> parent;
  uint64_t responses;
  uint64_t result;

  Fibonacci(): parent(None{}), responses(0), result(0) {}

  Fibonacci(cown_ptr<Fibonacci> parent): parent(parent), responses(0), result(0) {}

  static void root(int64_t n) { Fibonacci::compute(make_cown<Fibonacci>(), n); }

  static void request(cown_ptr<Fibonacci> parent, int64_t n) { Fibonacci::compute(make_cown<Fibonacci>(parent), n); }

  static void response(cown_ptr<Fibonacci> self, uint64_t n) {
    when(self) << [n](acquired_cown<Fibonacci> self) {
      self->result += n;
      self->responses++;

      if (self->responses == 2)
        self->propagate();
    };
  }

  static void compute(cown_ptr<Fibonacci> self, int64_t n) {
    when(self) << [tag=self, n](acquired_cown<Fibonacci> self) {
      if (n <= 2) {
        self->result = 1;
        self->propagate();
      } else {
        Fibonacci::request(tag, n-1);
        Fibonacci::request(tag, n-2);
      }
    };
  }

  struct Propagate {
    uint64_t result;
    void operator()(None)       { /* done */ }
    void operator()(cown_ptr<Fibonacci> parent)   { Fibonacci::response(parent, result); }
  };

  void propagate() { visit(Propagate{result}, parent); }

};

struct Fib: public AsyncBenchmark {
  uint64_t index;

  Fib(uint64_t index): index(index) {}

  void run() { Fibonacci::root(index); }

  string name() { return "Fib"; }

};

};

};