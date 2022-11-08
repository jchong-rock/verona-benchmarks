#include <cpp/when.h>
#include "util/bench.h"
#include "util/random.h"

namespace boc_benchmark {

namespace count {

using namespace std;

struct Producer;

struct Counter {
  uint64_t count = 0;
};

struct Producer {
  uint64_t messages;

  Producer(uint64_t messages): messages(messages) { }

  static void make(cown_ptr<Counter> counter, uint64_t messages) {
    for (uint64_t i = 0; i < messages; ++i) {
      when(counter) << [](acquired_cown<Counter> counter) { counter->count++; };
    }

    cown_ptr<Producer> producer = make_cown<Producer>(messages);
    when(counter, producer) << [](acquired_cown<Counter> counter, acquired_cown<Producer> producer) {
      /* done */
    };
  }
};

};

struct Count: BocBenchmark {
  uint64_t messages;

  Count(uint64_t messages): messages(messages) {}

  void run() { count::Producer::make(make_cown<count::Counter>(), messages); }

  std::string name() { return "Count"; }

};

};