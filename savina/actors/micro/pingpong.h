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
  static void ping(cown_ptr<Pong>, cown_ptr<Ping>);
};

struct Ping {
  uint64_t left;
  cown_ptr<Pong> _pong;

  Ping(uint64_t pings, cown_ptr<Pong> pong): left(pings - 1), _pong(pong) {}

  static void make(uint64_t pings, cown_ptr<Pong> pong) {
    Pong::ping(pong, make_cown<Ping>(pings, pong));
  }

  static void pong(cown_ptr<Ping> self) {
    when(self) << [tag=self](acquired_cown<Ping> self) {
      if(self->left > 0) {
        Pong::ping(self->_pong, tag);
        self->left--;
      } else {
        /* done */
      }
    };
  }
};

void Pong::ping(cown_ptr<Pong> self, cown_ptr<Ping> sender) {
  when(self) << [sender](acquired_cown<Pong> self) {
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

// class iso PingPong is AsyncActorBenchmark
//   let _pings: U64

//   new iso create(pings: U64) =>
//     _pings = pings
  
//   fun box apply(c: AsyncBenchmarkCompletion, last: Bool) =>
//     Ping(c, _pings, Pong)

//   fun tag name(): String => "Ping Pong"

// actor Pong
//   var _count: U64 = 0

//   be ping(sender: Ping) =>
//     sender.pong()
//     _count = _count + 1
