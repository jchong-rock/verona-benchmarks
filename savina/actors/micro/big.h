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
    : master(move(master)), index(index), random(index), pings(pings), sent(0) {}

  static void set_neighbors(const cown_ptr<BigActor>& self, vector<cown_ptr<BigActor>> n) {
    when(self) << [n=move(n)](acquired_cown<BigActor> self) mutable {
      self->neighbors = move(n);
    };
  }

  static void ping(const cown_ptr<BigActor>& self, int64_t sender) {
    when(self) << [sender](acquired_cown<BigActor> self) mutable {
      BigActor::pong(self->neighbors[sender], self->index);
    };
  }

  static void pong(const cown_ptr<BigActor>& self, int64_t n);

  static void cleanup(const cown_ptr<BigActor>& self) {
    when(self) << [](acquired_cown<BigActor> self) mutable {
      self->neighbors.clear();
    };
  }
};

struct BigMaster {
  uint64_t actors;

  // Keep track of actors so we can clean them up later
  vector<cown_ptr<BigActor>> n;

  BigMaster(uint64_t actors): actors(actors) {}

  static void make(uint64_t pings, uint64_t actors) {
    cown_ptr<BigMaster> master = make_cown<BigMaster>(actors);

    when(master) << [tag=move(master), pings, actors](acquired_cown<BigMaster> master) {
      for (uint64_t i = 0; i < actors; ++i)
        master->n.emplace_back(make_cown<BigActor>(tag, i, pings));

      for (const cown_ptr<BigActor>& big: master->n) {
        BigActor::set_neighbors(big, master->n);
      }

      for (const cown_ptr<BigActor>& big: master->n) {
        BigActor::pong(big, -1);
      }
    };
  }

  static void done(const cown_ptr<BigMaster>& self) {
    when(self) << [](acquired_cown<BigMaster> self)  mutable{
      if(--self->actors == 0) {
        for (const cown_ptr<BigActor>& actor: self->n)
          BigActor::cleanup(actor);
        self->n.clear();
      }
    };
  }
};

void BigActor::pong(const cown_ptr<BigActor>& self, int64_t n) {
  when(self) << [n](acquired_cown<BigActor> self) mutable{
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

struct Big: ActorBenchmark {
  uint64_t pings;
  uint64_t actors;

  Big(uint64_t pings, uint64_t actors): pings(pings), actors(actors) {}

  void run() {
    big::BigMaster::make(pings, actors);
  }

  std::string name() { return "Big"; }
};

};



