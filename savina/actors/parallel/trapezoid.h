#include <cpp/when.h>
#include <util/bench.h>
#include <util/random.h>
#include <cmath>

namespace actor_benchmark {

namespace trapezoid {

using namespace std;

struct Master;

struct Worker {
  static void create(const cown_ptr<Master>& master, double left, double right, double precision);
};

struct Master {
  double result_area;
  uint64_t workers;

  static cown_ptr<Master> create(uint64_t workers, double left, double right, double precision) {
    auto master = make_cown<Master>();
    when(master) << [tag=master, workers, left, right, precision](acquired_cown<Master> master) mutable {
      auto range = (right-left)/workers;

      for (uint64_t i = 0; i < workers; ++i) {
        left += (range * i);
        Worker::create(tag, left, left + range, precision);
      }
    };
    return master;
  }

  static void result(const cown_ptr<Master>& self, double area) {
    when(self) << [area](acquired_cown<Master> self) mutable {
      self->result_area += area;

      if (--self->workers == 0) {
        /* done */
      }
    };
  }
};

namespace Fx {
  double apply(double x) {
    auto a = sin(pow(x, 3.0) - 1.0);
    auto b = x + 1.0;
    auto c = a / b;
    auto d = sqrt(exp(sqrt(2 * x)) + 1);

    return c * d;
  }
}

// Question: is new in Pony async?? because this method was the actor constructor.
void Worker::create(const cown_ptr<Master>& master, double left, double right, double precision) {
  auto worker = make_cown<Worker>();
  when(worker) << [master, left, right, precision](acquired_cown<Worker> worker) mutable {
    double n = ((right - left) / precision);
    double accumulated_area = 0.0;

    double i = 0.0;
    while (i < n) {
      auto lx = (i * precision) + left;
      auto rx = lx + precision;

      auto ly = Fx::apply(lx);
      auto ry = Fx::apply(rx);

      accumulated_area += (0.5 * (ly + ry) * precision);

      i++;
    }

    Master::result(master, accumulated_area);
  };
}

};

struct Trapezoid: public AsyncBenchmark {
  uint64_t pieces;
  uint64_t workers;
  uint64_t left;
  uint64_t right;
  double precision;

  Trapezoid(uint64_t pieces, uint64_t workers, uint64_t left, uint64_t right):
    pieces(pieces), workers(workers), left(left), right(right), precision(double(right - left) / (double)pieces) {}

  void run() { trapezoid::Master::create(workers, left, right, precision); }

  std::string name() { return "Trapezoid"; }
};

};