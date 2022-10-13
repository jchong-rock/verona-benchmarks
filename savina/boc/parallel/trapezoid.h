#include <cpp/when.h>
#include <util/bench.h>
#include <util/random.h>
#include <cmath>

namespace boc_benchmark {

namespace trapezoid {

using namespace std;

namespace Worker {
  cown_ptr<double> create(double left, double right, double precision);
};

namespace Master {
  double result_area;
  uint64_t workers;

  cown_ptr<double> create(uint64_t workers, double left, double right, double precision) {
    // sort of map reduce
    auto range = (right-left)/workers;

    std::vector<cown_ptr<double>> partial_results;
    for (uint64_t i = 0; i < workers; ++i) {
      left += (range * i);
      partial_results.emplace_back(Worker::create(left, left + range, precision));
    }

#if true
   // two ways of reducing but both seem equally in speed:
   // - first is pair the 0th elem with every element of the list, so n messages but must be pipelined
   // - second creates a tree like reduction, has more parallelism but more messages
    auto pr = partial_results.begin();
    while (++pr != partial_results.end()) {
      when(*partial_results.begin(), *pr) << [](acquired_cown<double> r, acquired_cown<double> pr) {
        *r += *pr;
      };
    }
#else
    uint64_t its = ceil(log2(partial_results.size() + 1));

    for (uint64_t j = 0; j < its; ++j) {
      for (uint64_t i = 0; i < partial_results.size(); i += (1 << (j + 1))) {
        if (i + (1 << j) < partial_results.size()) {
          when(partial_results[i], partial_results[i + (1 << j)]) << [](acquired_cown<double> r, acquired_cown<double> pr) {
            *r += *pr;
          };
        }
      }
    }
#endif
    return partial_results[0];
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
cown_ptr<double> Worker::create(double left, double right, double precision) {
  auto result = make_cown<double>();
  when(result) << [left, right, precision](acquired_cown<double> result) mutable {
    double n = ((right - left) / precision);
    *result = 0.0;

    double i = 0.0;
    while (i < n) {
      auto lx = (i * precision) + left;
      auto rx = lx + precision;

      auto ly = Fx::apply(lx);
      auto ry = Fx::apply(rx);

      *result += (0.5 * (ly + ry) * precision);

      i++;
    }
  };
  return result;
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