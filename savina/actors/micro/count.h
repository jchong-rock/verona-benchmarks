#include <cpp/when.h>
#include "util/bench.h"
#include "util/random.h"

namespace actor_benchmark {

namespace count {

using namespace std;

struct Producer;

struct Counter {
  uint64_t count = 0;
  static void increment(const cown_ptr<Counter>&);
  static void retrieve(const cown_ptr<Counter>&, const cown_ptr<Producer>&);
};

struct Producer {
  uint64_t messages;

  Producer(uint64_t messages): messages(messages) { }

  static void make(const cown_ptr<Counter>& counter, uint64_t messages) {
    cown_ptr<Producer> producer = make_cown<Producer>(messages);
    for (uint64_t i = 0; i < messages; ++i) {
      Counter::increment(counter);
    }

    Counter::retrieve(counter, producer);
  }

  static void result(const cown_ptr<Producer>& self, uint64_t result) {
    when(self) << [](acquired_cown<Producer> self) mutable {
      /* done */
    };
  }
};

void Counter::increment(const cown_ptr<Counter>& self) {
  when(self) << [](acquired_cown<Counter> self)  mutable { self->count++; };
}

void Counter::retrieve(const cown_ptr<Counter>& self, const cown_ptr<Producer>& sender) {
  when(self) << [sender](acquired_cown<Counter> self)  mutable { Producer::result(sender, self->count); };
}

};

struct Count: AsyncBenchmark {
  uint64_t messages;

  Count(uint64_t messages): messages(messages) {}

  void run() { count::Producer::make(make_cown<count::Counter>(), messages); }

  std::string name() { return "Count"; }

};

};