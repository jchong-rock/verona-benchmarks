#include <cpp/when.h>
#include <util/bench.h>
#include <util/random.h>

namespace actor_benchmark {

namespace sieve {
  /*
   * Luke's understanding of how this benchmark works:
   *  - create an initial prime filter; that is
   *      - an actor with a buffer for prime numbers
   *      - a value is tested against these primes to see if it is a multiple of any known primes
   *      - if the value is a multiple then discard it and process the next value.
   *      - if the value is not and this actor still has space to store more primes, then store it
   *        and process the next value.
   *      - if the value is not and this actor has no more space to store primes (the value may
   *        not be a prime but just not a multiple of a prime known to this actor) in which case
   *        (possibly first create a next filter for larger primes and then) send to the next actor
   *        in the pipeline to test the value against larger primes and process the new value.
   *      - I think this means we get a pipeline of checking filters.
   *  - feed odd 3..N numbers to the initial prime filter (but does not aggregate the results).
   *
   */

using namespace std;

struct PrimeFilter {
  uint64_t size;
  uint64_t available;
  cown_ptr<PrimeFilter> next;
  vector<uint64_t> locals;

  PrimeFilter(uint64_t size): PrimeFilter(2, size) {}

  PrimeFilter(uint64_t initial, uint64_t size): size(size), available(1), locals(size, 0) {
    locals[0] = initial;
  }

  bool is_local(uint64_t value) {
    for (uint64_t i = 0; i < available; ++i) {
      // check if the value is a multiple of any known primes, if so then it is not prime
      if (value % locals[i] == 0)
        return false;
    }
    return true;
  }

  void handle_prime(uint64_t value) {
    if (available < size) {
      locals[available++] = value;
    } else {
      next = make_cown<PrimeFilter>(value, size);
    }
  }

  static void check_value(const cown_ptr<PrimeFilter>& self, uint64_t value) {
    when(self) << [value](acquired_cown<PrimeFilter> self)  mutable{
      if (self->is_local(value)) {
        if (self->next)
          PrimeFilter::check_value(self->next, value);
        else
          self->handle_prime(value);
      }
    };
  }

  static void done(const cown_ptr<PrimeFilter>& self) {
    when(self) << [](acquired_cown<PrimeFilter> self)  mutable{
      if (self->next)
        PrimeFilter::done(self->next);
    };
  }
};

namespace NumberProducer {
  void create(uint64_t size, const cown_ptr<PrimeFilter>& filter) {
    uint64_t candidate = 3;

    while (candidate < size) {
      PrimeFilter::check_value(filter, candidate);
      candidate += 2;
    }

    PrimeFilter::done(filter);
  }
};

};

struct Sieve: public AsyncBenchmark {
  uint64_t size;
  uint64_t buffersize;

  Sieve(uint64_t size, uint64_t buffersize): size(size), buffersize(buffersize) {}

  void run() {
    using namespace sieve;
    NumberProducer::create(size, make_cown<PrimeFilter>(buffersize));
  }

  std::string name() { return "Sieve of Eratosthenes"; }
};

};