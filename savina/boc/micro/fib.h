#include <memory>
#include <debug/harness.h>
#include <cpp/when.h>
#include "util/bench.h"
#include <random>

namespace boc_benchmark {

namespace fib {

using namespace std;

struct Fibonacci {

  static cown_ptr<uint64_t> compute(uint64_t n)
  {
    if (n <= 2) {
      return make_cown<uint64_t>(uint64_t{1});
    } else {
      cown_ptr<uint64_t> f1 = Fibonacci::compute(n - 1);
      when(f1, Fibonacci::compute(n - 2)) << [](acquired_cown<uint64_t> f1, acquired_cown<uint64_t> f2) { *f1 += * f2; };
      return f1;
    }
  }
};

};

struct Fib: public BocBenchmark {
  uint64_t index;

  Fib(uint64_t index): index(index) {}

  void run() { 
    auto result = fib::Fibonacci::compute(index); 
    when (result) << [=](acquired_cown<uint64_t> res) {
      std::cout << "Fib " << index << " = " << res << std::endl;
    };
  }

  inline static const std::string name = "Fib";

};

};