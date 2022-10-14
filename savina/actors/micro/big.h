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
  shared_ptr<const vector<cown_ptr<BigActor>>> neighbors;
  uint64_t sent;

  BigActor(cown_ptr<BigMaster> master, int64_t index, uint64_t pings)
    : master(move(master)), index(index), random(index), pings(pings), sent(0) {}

  static void set_neighbors(const cown_ptr<BigActor>& self, shared_ptr<const vector<cown_ptr<BigActor>>> n) {
    when(self) << [n=move(n)](acquired_cown<BigActor> self) mutable {
      self->neighbors = move(n);
    };
  }

  static void ping(const cown_ptr<BigActor>& self, int64_t sender) {
    when(self) << [sender](acquired_cown<BigActor> self) mutable {
      BigActor::pong((*self->neighbors)[sender], self->index);
    };
  }

  static void pong(const cown_ptr<BigActor>& self, int64_t n);
};

struct BigMaster {
  uint64_t actors;

  // Keep track of actors so we can clean them up later
  shared_ptr<vector<cown_ptr<BigActor>>> n;

  BigMaster(uint64_t actors): actors(actors), n() {}

  static cown_ptr<BigMaster> make(uint64_t pings, uint64_t actors) {
    cown_ptr<BigMaster> master = make_cown<BigMaster>(actors);

    when(master) << [pings, actors](acquired_cown<BigMaster> master) {
      vector<cown_ptr<BigActor>> n;

      for (uint64_t i = 0; i < actors; ++i) {
        auto a = make_cown<BigActor>(master.cown(), i, pings);
        n.emplace_back(move(a));
      }

      master->n = make_shared<vector<cown_ptr<BigActor>>>(move(n));

      for (const cown_ptr<BigActor>& big: *master->n) {
        BigActor::set_neighbors(big, master->n);
      }

      for (const cown_ptr<BigActor>& big: *master->n) {
        BigActor::pong(big, -1);
      }
    };
    return master;
  }

  static void done(const cown_ptr<BigMaster>& self) {
    when(self) << [](acquired_cown<BigMaster> self)  mutable{
      if(--self->actors == 0) {
        self->n->clear();
      }
    };
  }
};

void BigActor::pong(const cown_ptr<BigActor>& self, int64_t n) {
  when(self) << [n](acquired_cown<BigActor> self) mutable{
    if (self->sent < self->pings) {
      uint64_t index = self->random.nextInt(self->neighbors->size());
      BigActor::ping((*self->neighbors)[index], self->index);
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



