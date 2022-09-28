#include <cpp/when.h>
#include <util/bench.h>
#include <util/random.h>
#include <cmath>
#include <unordered_map>
#include <variant>
#include <tuple>

namespace actor_benchmark {

namespace radixsort {

using namespace std;

struct Sorter;
struct Validation;

using Neighbor = variant<cown_ptr<Validation>, cown_ptr<Sorter>>;

struct Validation {
  uint64_t size;
  double sum;
  uint64_t received;
  uint64_t previous;
  tuple<int64_t, int32_t> error;

  Validation(uint64_t size): size(size), sum(0), received(0), previous(0), error(make_tuple(0, 0)) {}

  static void value(cown_ptr<Validation> self, uint64_t n) {
    when(self) << [n](acquired_cown<Validation> self) {
      self->received++;
      if (n < self->previous && get<1>(self->error) < 0) {
        self->error = make_tuple(n, self->received - 1);
      }

      self->previous = n;
      self->sum += self->previous;

      if (self->received == self->size) {
        /* done */
      }
    };
  }
};

struct Sorter {
  Neighbor next;
  uint64_t size;
  uint64_t radix;
  vector<uint64_t> data;
  uint64_t received;
  uint64_t current;

  Sorter(uint64_t size, uint64_t radix, Neighbor next):
    next(next), size(size), radix(radix), data(size, 0), received(0), current(0) {}

  static void value(cown_ptr<Sorter> self, uint64_t n) {
    when(self) << [n](acquired_cown<Sorter> self){
      self->received++;

      if ((n & self->radix) == 0) {
        visit(overloaded {
          [&](cown_ptr<Validation>& next) { Validation::value(next, n); },
          [&](cown_ptr<Sorter>& next) { Sorter::value(next, n); },
        }, self->next);
      } else {
        self->data[self->current++] = n;
      }

      if (self->received == self->size) {
        for(uint64_t i = 0 ; i < self->current; ++i) {
          visit(overloaded {
            [&](cown_ptr<Validation>& next) { Validation::value(next, self->data[i]); },
            [&](cown_ptr<Sorter>& next) { Sorter::value(next, self->data[i]); },
          }, self->next);
        }
      }

    };
  }
};

namespace Source {
  void create(uint64_t size, uint64_t max, uint64_t seed, Neighbor next) {
    auto random = SimpleRand(seed);

    for (uint64_t i = 0; i < size; ++i) {
      visit(overloaded {
        [&](cown_ptr<Validation>& next) { Validation::value(next, random.nextLong() % max); },
        [&](cown_ptr<Sorter>& next) { Sorter::value(next, random.nextLong() % max); },
      }, next);
    }
  }
};

};

struct Radixsort: public AsyncBenchmark {
  uint64_t dataset;
  uint64_t max;
  uint64_t seed;

  Radixsort(uint64_t dataset, uint64_t max, uint64_t seed):
    dataset(dataset), max(max), seed(seed) {}

  void run() {
    using namespace radixsort;

    uint64_t radix = max / 2;
    Neighbor next = make_cown<Validation>(dataset);

    while (radix > 0) {
      next = make_cown<Sorter>(dataset, radix, next);
      radix /= 2;
    }

    Source::create(dataset, max, seed, next);
  }

  std::string name() { return "Radixsort"; }
};

};
