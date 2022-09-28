#include <memory>
#include <cpp/when.h>
#include "util/bench.h"
#include "util/random.h"

namespace actor_benchmark {

namespace big {

using namespace std;

struct BigMaster;

struct BigActor {
  cown_ptr<BigMaster> master;
  int64_t index;
  SimpleRand random;
  uint64_t pings;
  vector<cown_ptr<BigActor>> neighbors;
  uint64_t sent;

  BigActor(cown_ptr<BigMaster> master, int64_t index, uint64_t pings)
    : master(master), index(index), random(index), pings(pings), sent(0) {}

  static void set_neighbors(cown_ptr<BigActor> self, vector<cown_ptr<BigActor>> n) {
    when(self) << [n](acquired_cown<BigActor> self) mutable {
      self->neighbors = n;
    };
  }

  static void ping(cown_ptr<BigActor> self, int64_t sender) {
    when(self) << [sender](acquired_cown<BigActor> self) mutable {
      BigActor::pong(self->neighbors[sender], self->index);
    };
  }

  static void pong(cown_ptr<BigActor> self, int64_t n);
};

struct BigMaster {
  uint64_t actors;

  BigMaster(uint64_t actors): actors(actors) {}

  static void make(uint64_t pings, uint64_t actors) {
    cown_ptr<BigMaster> master = make_cown<BigMaster>(actors);

    std::vector<cown_ptr<BigActor>> n;

    for (uint64_t i = 0; i < actors; ++i)
      n.push_back(make_cown<BigActor>(master, i, pings));

    for (cown_ptr<BigActor> big: n) {
      BigActor::set_neighbors(big, n);
    }

    for (cown_ptr<BigActor> big: n) {
      BigActor::pong(big, -1);
    }
  }

  static void done(cown_ptr<BigMaster> self) {
    when(self) << [](acquired_cown<BigMaster> self) {
      if(--self->actors == 0) {
        /* done */
      }
    };
  }
};

void BigActor::pong(cown_ptr<BigActor> self, int64_t n) {
  when(self) << [n](acquired_cown<BigActor> self) mutable {
    if (self->sent < self->pings) {
      uint64_t index = self->random.nextInt(self->neighbors.size());
      BigActor::ping(self->neighbors[index], self->index);
      self->sent++;
    } else {
      BigMaster::done(self->master);
    }
  };
}

};

struct Big: AsyncBenchmark {
  uint64_t pings;
  uint64_t actors;

  Big(uint64_t pings, uint64_t actors): pings(pings), actors(actors) {}

  void run() {
    big::BigMaster::make(pings, actors);
  }

  std::string name() { return "Big"; }
};

};



