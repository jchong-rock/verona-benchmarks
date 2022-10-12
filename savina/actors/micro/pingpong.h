#include <cpp/when.h>
#include "util/bench.h"
#include "util/random.h"
#include <cmath>

namespace actor_benchmark {

namespace pingpong {

using namespace std;

struct Ping;

struct Pong {
  uint64_t count = 0;
  static void ping(const cown_ptr<Pong>&, cown_ptr<Ping>);
};

struct Ping {
  uint64_t left;
  cown_ptr<Pong> _pong;

  Ping(uint64_t pings, const cown_ptr<Pong>& pong): left(pings - 1), _pong(pong) {}

  static void make(uint64_t pings, cown_ptr<Pong> pong) {
    Pong::ping(pong, make_cown<Ping>(pings, pong));
  }

  static void pong(const cown_ptr<Ping>& self) {
    when(self) << [tag=self](acquired_cown<Ping> self)  mutable{
      if(self->left > 0) {
        Pong::ping(self->_pong, move(tag));
        self->left--;
      } else {
        /* done */
      }
    };
  }
};

void Pong::ping(const cown_ptr<Pong>& self, cown_ptr<Ping> sender) {
  when(self) << [sender=move(sender)](acquired_cown<Pong> self)  mutable{
    Ping::pong(sender);
    self->count++;
  };
}

};

struct PingPong: public AsyncBenchmark {
  uint64_t pings;

  PingPong(uint64_t pings): pings(pings) {}

  void run() { pingpong::Ping::make(pings, make_cown<pingpong::Pong>()); }

  std::string name() { return "Ping Pong"; }
};

};