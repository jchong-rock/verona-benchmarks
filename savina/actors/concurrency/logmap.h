#include "util/bench.h"
#include "util/random.h"

namespace actor_benchmark {

namespace logmap {

using namespace std;

struct SeriesWorker;
struct LogmapMaster;

struct RateComputer {
  double rate;

  RateComputer(double rate): rate(rate) {}

  static void compute(const cown_ptr<RateComputer>&, cown_ptr<SeriesWorker>, double);
};

enum class StashedMessage { NextMessage, GetMessage };

struct SeriesWorker {
  cown_ptr<LogmapMaster> master;
  cown_ptr<RateComputer> computer;
  double term;
  deque<StashedMessage> buffer;
  bool stash_mode;

  SeriesWorker(cown_ptr<LogmapMaster>& master, cown_ptr<RateComputer>&& computer, double term): master(master), computer(move(computer)), term(term), stash_mode(false) {}

  static void next(const cown_ptr<SeriesWorker>&);
  static void result(const cown_ptr<SeriesWorker>&, double);
  static void get(const cown_ptr<SeriesWorker>&);

  void stash(StashedMessage message) { buffer.push_back(message); }
  bool unstash(cown_ptr<SeriesWorker>, double);
};

void RateComputer::compute(const cown_ptr<RateComputer>& self, cown_ptr<SeriesWorker> worker, double term) {
  when(self) << [worker=move(worker), term](acquired_cown<RateComputer> self)  mutable {
    SeriesWorker::result(worker, self->rate * term * (1 - term));
  };
}

struct LogmapMaster {
  std::deque<cown_ptr<SeriesWorker>> workers;
  uint64_t terms;
  uint64_t requested;
  uint64_t received;
  double sum;

  LogmapMaster(uint64_t terms): terms(terms), requested(0), received(0), sum(0) {}

  static cown_ptr<LogmapMaster> make(uint64_t terms, uint64_t series, double rate, double increment) {
    cown_ptr<LogmapMaster> master = make_cown<LogmapMaster>(terms);
    when(master) << [tag=master,terms, series, rate, increment](acquired_cown<LogmapMaster> master)  mutable {
      for (uint64_t j = 0; j < series; ++j) {
        double start_term = (double)j * increment;
        master->workers.emplace_back(make_cown<SeriesWorker>(tag, make_cown<RateComputer>(rate + start_term), start_term));

      }
    };
    return master;
  }

  static void start(const cown_ptr<LogmapMaster>& self) {
    when(self) << [](acquired_cown<LogmapMaster> self)  mutable {
      for(uint64_t i = 0; i < self->terms; ++i)
        for(uint64_t j = 0; j < self->workers.size(); ++j)
          SeriesWorker::next(self->workers[j]);

      for(uint64_t k = 0; k < self->workers.size(); ++k) {
        SeriesWorker::get(self->workers[k]);
        self->requested++;
      }
    };
  }

  static void result(const cown_ptr<LogmapMaster>& self, double term) {
    when(self) << [term](acquired_cown<LogmapMaster> self)  mutable {
      self->sum += term;
      self->received++;
      if(self->received == self->requested) {
        self->workers.clear();
        // std::cout << self->sum << std::endl;
        /* done */
      }
    };
  }
};

bool SeriesWorker::unstash(cown_ptr<SeriesWorker> self, double term) {
  while (!buffer.empty())
  {
    StashedMessage message = buffer.front();
    buffer.pop_front();
    switch (message)
    {
    case StashedMessage::NextMessage:
      RateComputer::compute(computer, move(self), term);
      return true;

    case StashedMessage::GetMessage:
      if (buffer.empty()) {
        LogmapMaster::result(master, term);
      } else {
        buffer.push_back(message);
      }
      break;
    default:
      break;
    }
  }
  return false;
}


void SeriesWorker::next(const cown_ptr<SeriesWorker>& self) {
  when(self) << [tag=self](acquired_cown<SeriesWorker> self)  mutable {
    if (self->stash_mode) {
      self->stash(StashedMessage::NextMessage);
    } else {
      RateComputer::compute(self->computer, move(tag), self->term);
      self->stash_mode = true;
    }
  };
}

void SeriesWorker::result(const cown_ptr<SeriesWorker>& self, double term) {
  when(self) << [tag=self, term](acquired_cown<SeriesWorker> self)  mutable {
    self->term = term;
    if (self->stash_mode) {
      self->stash_mode = self->unstash(move(tag), term);
    }
  };
}

void SeriesWorker::get(const cown_ptr<SeriesWorker>& self) {
  when(self) << [](acquired_cown<SeriesWorker> self)  mutable {
    if (self->stash_mode) {
      self->stash(StashedMessage::GetMessage);
    } else {
      LogmapMaster::result(self->master, self->term);
    }
  };
}

};

struct Logmap: public AsyncBenchmark {
  uint64_t terms;
  uint64_t series;
  double rate;
  double increment;

  Logmap(uint64_t terms, uint64_t series, double rate, double increment): terms(terms), series(series), rate(rate), increment(increment) {}

  void run() {
    using namespace logmap;
    LogmapMaster::start(LogmapMaster::make(terms, series, rate, increment));
  }

  std::string name() { return "Logistic Map Series"; }

};

};


