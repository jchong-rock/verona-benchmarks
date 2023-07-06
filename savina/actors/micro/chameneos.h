#include <memory>
#include <cpp/when.h>
#include "util/bench.h"
#include "util/random.h"

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

enum class None {};

struct Mall;

struct Chameneo {
  cown_ptr<Mall> mall;
  ChameneoColor color;
  uint64_t meeting_count;

  Chameneo(const cown_ptr<Mall>& mall, ChameneoColor color): mall(mall), color(color), meeting_count(0) {}

  static void make(const cown_ptr<Mall>& mall, ChameneoColor color);
  static void meet(const cown_ptr<Chameneo>& self, cown_ptr<Chameneo> approaching, ChameneoColor color);
  static void change(const cown_ptr<Chameneo>& self, ChameneoColor color);
  static void report(const cown_ptr<Chameneo>& self);
};

struct Mall {
  uint64_t chameneos;
  uint64_t faded;
  uint64_t meeting_count;
  uint64_t sum;
  cown_ptr<Chameneo> waiting;

  Mall(uint64_t meetings, uint64_t chameneos)
    : chameneos(chameneos), faded(0), meeting_count(meetings), sum(0) {}

  static void make(uint64_t meetings, uint64_t chameneos) {
    cown_ptr<Mall> mall = make_cown<Mall>(meetings, chameneos);
    for (uint64_t i = 0; i < chameneos; ++i)
      Chameneo::make(mall, Color::factory(i % 3));
  }

  static void meetings(const cown_ptr<Mall>& self, uint64_t count) {
    when(self) << [count](acquired_cown<Mall> self)  mutable{
      self->faded++;
      self->sum += count;

      if (self->faded == self->chameneos) {
        /* done */
      }
    };
  }

  static void meet(const cown_ptr<Mall>& self, cown_ptr<Chameneo> approaching, ChameneoColor color) {
    when(self) << [approaching=move(approaching), color](acquired_cown<Mall> self)  mutable{
      if (self->meeting_count > 0) {
        if(!self->waiting) {
          self->waiting = move(approaching);
        } else {
          self->meeting_count--;
          Chameneo::meet(self->waiting, move(approaching), color);
          self->waiting = nullptr; // cown_ptr<Chameneo>();
        }
      } else {
        Chameneo::report(approaching);
      }
    };
  }
};

void Chameneo::make(const cown_ptr<Mall>& mall, ChameneoColor color) {
  Mall::meet(mall, make_cown<Chameneo>(mall, color), color);
}

void Chameneo::meet(const cown_ptr<Chameneo>& self, cown_ptr<Chameneo> approaching, ChameneoColor color) {
  when(self) << [tag=self, approaching=move(approaching), color](acquired_cown<Chameneo> self)  mutable{
    self->color = Color::complement(self->color, color);
    self->meeting_count++;
    Chameneo::change(approaching, self->color);
    Mall::meet(self->mall, tag, self->color);
  };
}

void Chameneo::change(const cown_ptr<Chameneo>& self, ChameneoColor color) {
  when(self) << [tag=self, color](acquired_cown<Chameneo> self)  mutable{
    self->color = color;
    self->meeting_count++;
    Mall::meet(self->mall, move(tag), self->color);
  };
}

void Chameneo::report(const cown_ptr<Chameneo>& self) {
  when(self) << [tag=self](acquired_cown<Chameneo> self)  mutable{
    Mall::meetings(self->mall, self->meeting_count);
    self->color = ChameneoColor::Faded;
  };
}

};

struct Chameneos: public ActorBenchmark {
  uint64_t meetings;
  uint64_t chameneos;

  Chameneos(uint64_t chameneos, uint64_t meetings): meetings(meetings), chameneos(chameneos) {}

  void run() { chameneos::Mall::make(meetings, chameneos); }

  inline static const std::string name = "Chameneos";
};

};