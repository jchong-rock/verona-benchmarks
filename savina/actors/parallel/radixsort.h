#include <cpp/when.h>
#include <util/bench.h>
#include <util/random.h>
#include <cmath>
#include <unordered_map>
#include <tuple>

namespace actor_benchmark {

namespace radixsort {

using namespace std;

// I believe this pipelined version will be incredibly slow to just a sequential version,
// because i think the only time two actors are working at the same time is when one is sending
// and one is processing. Experiments look to agree.

// I don't think we can do anything BoC-like here.
struct Sorter;
struct Validation;

struct Validation {
  uint64_t size;
  double sum;
  uint64_t received;
  uint64_t previous;
  tuple<int64_t, int32_t> error;

  Validation(uint64_t size): size(size), sum(0), received(0), previous(0), error(make_tuple(-1, -1)) {}

  static void value(const cown_ptr<Validation>& self, uint64_t n) {
    when(self) << [n](acquired_cown<Validation> self)  mutable {
      self->received++;
      if (n < self->previous && get<1>(self->error) < 0) {
        self->error = make_tuple(n, self->received - 1);
      }

      // self->data.push_back(n);

      self->previous = n;
      self->sum += self->previous;

      if (self->received == self->size) {
        // cout << "sorted: " << (is_sorted(self->data.begin(), self->data.end()) ? "true": "false") << endl;
        /* done */
      }
    };
  }
};

struct Sorter {
  cown_ptr<Validation> validation;
  cown_ptr<Sorter> sorter;
  uint64_t size;
  uint64_t radix;
  vector<uint64_t> data;
  uint64_t received;
  uint64_t current;

  Sorter(uint64_t size, uint64_t radix, cown_ptr<Validation> next):
    validation(move(next)), size(size), radix(radix), data(size, 0), received(0), current(0) {}

  Sorter(uint64_t size, uint64_t radix, cown_ptr<Sorter> next):
    sorter(move(next)), size(size), radix(radix), data(size, 0), received(0), current(0) {}

  // The pipeline seems intrinsic to the order

  static void value(const cown_ptr<Sorter>& self, uint64_t n) {
    when(self) << [n](acquired_cown<Sorter> self) mutable {
      self->received++;

      if ((n & self->radix) == 0) {
        if (self->validation == nullptr)
          Sorter::value(self->sorter, n);
        else
          Validation::value(self->validation, n);
      } else {
        self->data[self->current++] = n;
      }

      if (self->received == self->size) {
        if (self->validation == nullptr)
          for(uint64_t i = 0 ; i < self->current; ++i)
            Sorter::value(self->sorter, self->data[i]);
        else
          for(uint64_t i = 0 ; i < self->current; ++i)
            Validation::value(self->validation, self->data[i]);
      }
    };
  }
};

namespace Source {
  void create(uint64_t size, uint64_t max, uint64_t seed, const cown_ptr<Validation>& next) {
    auto random = SimpleRand(seed);
    for (uint64_t i = 0; i < size; ++i) {
      Validation::value(next, random.nextLong() % max);
    }
  }

  void create(uint64_t size, uint64_t max, uint64_t seed, const cown_ptr<Sorter>& next) {
    auto random = SimpleRand(seed);
    for (uint64_t i = 0; i < size; ++i) {
      Sorter::value(next, random.nextLong() % max);
    }
  }
};

};

struct Radixsort: public ActorBenchmark {
  uint64_t dataset;
  uint64_t max;
  uint64_t seed;

  Radixsort(uint64_t dataset, uint64_t max, uint64_t seed):
    dataset(dataset), max(max), seed(seed) {}

  void run() {
    using namespace radixsort;

    uint64_t radix = max / 2;
    cown_ptr<Validation> v = make_cown<Validation>(dataset);

    if (radix > 0) {
      cown_ptr<Sorter> next = make_cown<Sorter>(dataset, radix, move(v));
      radix /= 2;
      while (radix > 0) {
        next = make_cown<Sorter>(dataset, radix, move(next));
        radix /= 2;
      }

      Source::create(dataset, max, seed, next);
    } else {
      Source::create(dataset, max, seed, v);
    }
  }

  inline static const std::string name = "Radixsort";
};

};