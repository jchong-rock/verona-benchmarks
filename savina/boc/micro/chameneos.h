#include <memory>
#include <cpp/when.h>
#include "util/bench.h"
#include "util/random.h"

namespace boc_benchmark {

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

  Chameneo(cown_ptr<Mall> mall, ChameneoColor color): mall(mall), color(color), meeting_count(0) {}
  static void make(cown_ptr<Mall>, ChameneoColor);
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

  // No longer need to thread the color through the meeting calls (which in my opinion was an optimisation of the rendezvous in the original benchmark)
  static void meet(cown_ptr<Mall> mall, cown_ptr<Chameneo> approaching) {
    when(mall) << [approaching=move(approaching)](acquired_cown<Mall> mall) {
      if (mall->meeting_count > 0) {
        if (mall->waiting) {
          when(mall->waiting, approaching) << [](acquired_cown<Chameneo> a, acquired_cown<Chameneo> b) {
            a->color = b->color = Color::complement(a->color, b->color);

            a->meeting_count++;
            Mall::meet(a->mall, a.cown());

            b->meeting_count++;
            Mall::meet(b->mall, b.cown());
          };

          mall->meeting_count--;
          mall->waiting = nullptr;
        } else {
          mall->waiting = move(approaching);
        }
      } else {

        when(mall.cown(), approaching) << [](acquired_cown<Mall> mall, acquired_cown<Chameneo> approaching) {
          approaching->color = ChameneoColor::Faded;
          mall->sum += approaching->meeting_count;
          if (++mall->faded == mall->chameneos) {
            /* done */
          }
        };
      }
    };
  }
};

void Chameneo::make(cown_ptr<Mall> mall, ChameneoColor color) {
  auto c = make_cown<Chameneo>(mall, color);
  Mall::meet(move(mall), c);
}

};

struct Chameneos: public BocBenchmark {
  uint64_t meetings;
  uint64_t chameneos;

  Chameneos(uint64_t chameneos, uint64_t meetings): meetings(meetings), chameneos(chameneos) {}

  void run() { chameneos::Mall::make(meetings, chameneos); }

  std::string name() { return "Chameneos"; }
};

};