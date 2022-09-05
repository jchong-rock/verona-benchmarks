#include <memory>
#include <test/harness.h>
#include <cpp/when.h>
#include "util/bench.h"
#include <random>
#include <variant>

namespace BOCBenchmark {

namespace {

using namespace std;

struct Fibonacci {

  static cown_ptr<uint64_t> compute(uint64_t n)
  {
    if (n <= 2) {
      cown_ptr<uint64_t> result = make_cown<uint64_t>(uint64_t{1});
      return result;
    } else {
      cown_ptr<uint64_t> f1 = Fibonacci::compute(n - 1);
      cown_ptr<uint64_t> f2 = Fibonacci::compute(n - 2);
      when(f1, f2) << [](acquired_cown<uint64_t> f1, acquired_cown<uint64_t> f2) { *f1 += * f2; };
      return f1;
    }
  }

};

struct Fib: public AsyncBenchmark {
  uint64_t index;

  Fib(uint64_t index): index(index) {}

  void run() { Fibonacci::compute(index); }

  string name() { return "Fib"; }

};

};

};