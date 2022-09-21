#include <memory>
#include <cpp/when.h>
#include "util/bench.h"
#include "util/random.h"
#include <variant>

namespace actor_benchmark {

namespace chameneos {

using namespace std;

enum class ChameneoColor { Blue = 0, Red, Yellow, Faded };

namespace Color {
  ChameneoColor complement(ChameneoColor color, ChameneoColor other) {
    if (color == other) {
      return color;
    } else if (color == ChameneoColor::Faded || other == ChameneoColor::Faded) {
      return ChameneoColor::Faded;
    } else {
      return ChameneoColor(3 - (int)color - (int)other);
    }
  }

  ChameneoColor factory(uint64_t index) {
    // No idea why but this is the order and it's different from the enum class elsewhere
    assert(index <= 2);
    return ChameneoColor((index + 1) % 3);
  }
};

struct None {};

struct Mall;

struct Chameneo {
  cown_ptr<Mall> mall;
  ChameneoColor color;
  uint64_t meeting_count;

  Chameneo(cown_ptr<Mall> mall, ChameneoColor color): mall(mall), color(color), meeting_count(0) {}

  static void make(cown_ptr<Mall> mall, ChameneoColor color);
  static void meet(cown_ptr<Chameneo> self, cown_ptr<Chameneo> approaching, ChameneoColor color);
  static void change(cown_ptr<Chameneo> self, ChameneoColor color);
  static void report(cown_ptr<Chameneo> self);
};

struct Mall {
  uint64_t chameneos;
  uint64_t faded;
  uint64_t meeting_count;
  uint64_t sum;
  variant<cown_ptr<Chameneo>, None> waiting;

  Mall(uint64_t meetings, uint64_t chameneos)
    : chameneos(chameneos), faded(0), meeting_count(meetings), sum(0), waiting(None{}) {}

  static void make(uint64_t meetings, uint64_t chameneos) {
    cown_ptr<Mall> mall = make_cown<Mall>(meetings, chameneos);
    for (uint64_t i = 0; i < chameneos; ++i)
      Chameneo::make(mall, Color::factory(i % 3));
  }

  static void meetings(cown_ptr<Mall> self, uint64_t count) {
    when(self) << [count](acquired_cown<Mall> self) {
      self->faded++;
      self->sum += count;

      if (self->faded == self->chameneos) {
        /* done */
      }
    };
  }

  static void meet(cown_ptr<Mall> self, cown_ptr<Chameneo> approaching, ChameneoColor color) {
    when(self) << [approaching, color](acquired_cown<Mall> self) {
      if (self->meeting_count > 0) {
        visit(overloaded{
          [&](None) { self->waiting = approaching; },
          [&](cown_ptr<Chameneo> chameneo) {
            self->meeting_count--;
            Chameneo::meet(chameneo, approaching, color);
            self->waiting = None{};
          },
        }, self->waiting);
      } else {
        Chameneo::report(approaching);
      }
    };
  }
};

void Chameneo::make(cown_ptr<Mall> mall, ChameneoColor color) {
  Mall::meet(mall, make_cown<Chameneo>(mall, color), color);
}

void Chameneo::meet(cown_ptr<Chameneo> self, cown_ptr<Chameneo> approaching, ChameneoColor color) {
  when(self) << [tag=self, approaching, color](acquired_cown<Chameneo> self) {
    self->color = Color::complement(self->color, color);
    self->meeting_count++;
    Chameneo::change(approaching, self->color);
    Mall::meet(self->mall, tag, self->color);
  };
}

void Chameneo::change(cown_ptr<Chameneo> self, ChameneoColor color) {
  when(self) << [tag=self, color](acquired_cown<Chameneo> self) {
    self->color = color;
    self->meeting_count++;
    Mall::meet(self->mall, tag, self->color);
  };
}

void Chameneo::report(cown_ptr<Chameneo> self) {
  when(self) << [tag=self](acquired_cown<Chameneo> self) {
    Mall::meetings(self->mall, self->meeting_count);
    self->color = ChameneoColor::Faded;
  };
}

};

struct Chameneos: public AsyncBenchmark {
  uint64_t meetings;
  uint64_t chameneos;

  Chameneos(uint64_t chameneos, uint64_t meetings): meetings(meetings), chameneos(chameneos) {}

  void run() { chameneos::Mall::make(meetings, chameneos); }

  std::string name() { return "Chameneos"; }
};

};