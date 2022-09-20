#include "util/bench.h"
#include "util/random.h"

namespace BOCBenchmark {

namespace {

using namespace std;

struct RateComputer {
  double rate;

  RateComputer(double rate): rate(rate) {}

  double compute(double term) { return rate * term * (1 - term); }
};

struct SeriesWorker {
  double term;

  SeriesWorker(double term): term(term) {}

  static void next(cown_ptr<SeriesWorker> worker, cown_ptr<RateComputer> computer) {
    when(worker, computer) << [] (acquired_cown<SeriesWorker> worker, acquired_cown<RateComputer> computer){
      worker->term = computer->compute(worker->term);
    };
  }
};

namespace LogmapMaster {
  static void start(uint64_t terms, uint64_t series, double rate, double increment) {
    vector<cown_ptr<SeriesWorker>> workers;
    vector<cown_ptr<RateComputer>> computers;

    for (uint64_t j = 0; j < series; ++j) {
      double start_term = (double)j * increment;
      workers.push_back(make_cown<SeriesWorker>(start_term));
      computers.push_back(make_cown<RateComputer>(rate + start_term));
    }

    for(uint64_t i = 0; i < terms; ++i)
      for(uint64_t j = 0; j < workers.size(); ++j)
        SeriesWorker::next(workers[j], computers[j]);

    cown_ptr<double> sum = make_cown<double>(0);
    for(uint64_t k = 0; k < workers.size(); ++k) {
      when(sum, workers[k]) << [](acquired_cown<double> sum, acquired_cown<SeriesWorker> worker) {
        sum += worker->term;
      };
    }

    // when(sum) << [](acquired_cown<double> sum) { std::cout << *sum << std::endl; };
    /* done result is in sum*/
  }
};

struct Logmap: public AsyncBenchmark {
  uint64_t terms;
  uint64_t series;
  double rate;
  double increment;

  Logmap(uint64_t terms, uint64_t series, double rate, double increment): terms(terms), series(series), rate(rate), increment(increment) {}

  void run() {
    LogmapMaster::start(terms, series, rate, increment);
  }

  std::string name() { return "Logistic Map Series"; }

};

};

};


