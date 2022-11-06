#include <cpp/when.h>
#include <util/bench.h>
#include <random>
#include <cmath>
#include <unordered_map>

namespace actor_benchmark {

namespace filterbank {

using namespace verona::cpp;
using namespace std;

using Matrix = vector<vector<uint64_t>>;

struct Producer;
struct Branch;

struct Source {
  cown_ptr<Producer> producer;
  cown_ptr<Branch> branch;
  uint64_t max;
  uint64_t current;

  Source(const cown_ptr<Producer> producer, const cown_ptr<Branch> branch):
    producer(move(producer)), branch(move(branch)), max(1000), current(0) {}

  static void boot(const cown_ptr<Source>& self);
};

struct Producer {
  const uint64_t simulations;
  uint64_t sent;

  Producer(uint64_t simulations): simulations(simulations), sent(0) {}

  static void next(const cown_ptr<Producer>& self, cown_ptr<Source> source) {
    when(self) << [source=move(source)](acquired_cown<Producer> self)  mutable {
      if (self->sent < self->simulations) {
        Source::boot(source);
        self->sent++;
      } else {
        // complete
      }
    };
  }
};

struct Sink {
  const uint64_t sinkrate;
  uint64_t count;

  Sink(uint64_t sinkrate): sinkrate(sinkrate), count(0) {};

  static void value(const cown_ptr<Sink>& self, uint64_t n) {
    when(self) << [n](acquired_cown<Sink> self)  mutable {
      self->count = (self->count + 1) % self->sinkrate;
    };
  }
};

struct Combine {
  cown_ptr<Sink> sink;

  Combine(const cown_ptr<Sink>& sink): sink(sink) {}

  static void collect(const cown_ptr<Combine>& self, unique_ptr<unordered_map<uint64_t, uint64_t>> map) {
    when(self) << [map=std::move(map)](acquired_cown<Combine> self)  mutable {
      uint64_t sum = 0;

      for (auto item : *map) {
        sum += item.second;
      }

      Sink::value(self->sink, sum);
    };
  }
};

struct Integrator {
  const uint64_t channels;
  cown_ptr<Combine> combine;
  deque<unique_ptr<unordered_map<uint64_t, uint64_t>>> data;

  Integrator(uint64_t channels, cown_ptr<Combine> combine): channels(channels), combine(move(combine)) {}

  static void value(const cown_ptr<Integrator>& self, uint64_t id, uint64_t n) {
    when(self) << [id, n](acquired_cown<Integrator> self) mutable {
      bool processed = false;
      uint64_t size = self->data.size();
      uint64_t i = 0;

      while(i < size) {
        auto& data = self->data[i];
        if (data->find(id) == data->end()) {
          (*data)[id] = n;
          processed = true;
          i = size;
        }

        i++;
      }

      if (!processed) {
        auto new_map = make_unique<unordered_map<uint64_t, uint64_t>>();
        (*new_map)[id] = n;
        self->data.push_back(move(new_map));
      }

      if (self->data[0]->size() == self->channels) {
        auto first = std::move(self->data.front());
        self->data.pop_front();
        Combine::collect(self->combine, move(first));
      }
    };
  }
};

struct SampleFilter;
struct TaggedForward;

struct FirFilter {
  uint64_t length;
  vector<uint64_t> coefficients;
  cown_ptr<SampleFilter> sample;
  cown_ptr<TaggedForward> tagged;

  vector<uint64_t> data;
  uint64_t index;
  bool is_full;

  FirFilter(uint64_t length, vector<uint64_t> coefficients, cown_ptr<SampleFilter> next):
    length(length), coefficients(coefficients), sample(move(next)), index(0), data(length, 0), is_full(false) {}

  FirFilter(uint64_t length, vector<uint64_t> coefficients, cown_ptr<TaggedForward> next):
    length(length), coefficients(coefficients), tagged(move(next)), index(0), data(length, 0), is_full(false) {}

  static void value(const cown_ptr<FirFilter>& self, uint64_t n);

};

struct Delay {
  const uint64_t length;
  cown_ptr<FirFilter> filter;
  vector<uint64_t> state;
  uint64_t placeholder;

  Delay(uint64_t length, const cown_ptr<FirFilter> filter): length(length), filter(move(filter)), state(length, 0), placeholder(0) {}

  static void value(const cown_ptr<Delay>& self, uint64_t n) {
    when(self) << [n](acquired_cown<Delay> self)  mutable {
      FirFilter::value(self->filter, self->state[self->placeholder]);
      self->state[self->placeholder] = n;
      self->placeholder = (self->placeholder + 1) % self->length;
    };
  }
};

struct SampleFilter {
  const uint64_t rate;
  cown_ptr<Delay> delay;

  uint64_t samples_received;

  SampleFilter(uint64_t rate, cown_ptr<Delay> delay): rate(rate), delay(move(delay)), samples_received(0) {}

  static void value(const cown_ptr<SampleFilter>& self, uint64_t n) {
    when(self) << [n](acquired_cown<SampleFilter> self)  mutable {
      if (self->samples_received == 0) {
        Delay::value(self->delay, n);
      } else {
        Delay::value(self->delay, 0);
      }

      self->samples_received = (self->samples_received + 1) % self->rate;
    };
  }
};

struct TaggedForward {
  uint64_t id;
  cown_ptr<Integrator> integrator;

  TaggedForward(uint64_t id, cown_ptr<Integrator> integrator): id(id), integrator(move(integrator)) {}

  static void value(const cown_ptr<TaggedForward>& self, uint64_t n) {
    when(self) << [n](acquired_cown<TaggedForward> self)  mutable {
      Integrator::value(self->integrator, self->id, n);
    };
  }
};

struct Bank {
  cown_ptr<Delay> entry;

  Bank(uint64_t id, uint64_t columns, vector<uint64_t> h, vector<uint64_t> f, cown_ptr<Integrator> integrator)
    : entry(make_cown<Delay>(columns - 1,
              make_cown<FirFilter>(columns, h,
                make_cown<SampleFilter>(columns,
                  make_cown<Delay>(columns - 1,
                    make_cown<FirFilter>(columns, f,
                      make_cown<TaggedForward>(id, move(integrator)))))))) {}

  static void value(const cown_ptr<Bank>& self, uint64_t n) {
    when(self) << [n](acquired_cown<Bank> self)  mutable {
      Delay::value(self->entry, n);
    };
  }
};

struct Branch {
  uint64_t channels;
  uint64_t columns;
  Matrix h;
  Matrix f;
  cown_ptr<Integrator> integrator;
  vector<cown_ptr<Bank>> banks;

  Branch(uint64_t channels, uint64_t columns, Matrix h, Matrix f, const cown_ptr<Integrator>& integrator):
    channels(channels), columns(columns), h(h), f(f), integrator(move(integrator))
  {
    uint64_t index = 0;
    for (uint64_t i = 0; i < channels; ++i) {
      index = i;

      banks.emplace_back(make_cown<Bank>(i, columns, h[index], f[index], integrator));
    }
  }

  static void value(const cown_ptr<Branch>& self, uint64_t n) {
    when(self) << [n](acquired_cown<Branch> self)  mutable {
      for (const cown_ptr<Bank>& bank: self->banks) {
        Bank::value(bank, n);
      }
    };
  }
};

void Source::boot(const cown_ptr<Source>& self) {
  when(self) << [tag = self](acquired_cown<Source> self)  mutable {
    Branch::value(self->branch, self->current);
    self->current = (self->current + 1) % self->max;
    Producer::next(self->producer, move(tag));
  };
}

void FirFilter::value(const cown_ptr<FirFilter>& self, uint64_t n) {
  when(self) << [n](acquired_cown<FirFilter> self)  mutable {
    self->data[self->index] = n;
    self->index++;

    if (self->index == self->length) {
      self->is_full = true;
      self->index = 0;
    }

    if (self->is_full) {
      uint64_t sum = 0;
      uint64_t i = 0;

      while (i < self->length) {
        sum += self->data[i] * self->coefficients[self->length - i - 1];
        i++;
      }

      if (self->sample == nullptr) {
        TaggedForward::value(self->tagged, sum);
      } else {
        SampleFilter::value(self->sample, sum);
      }
    }
  };
}
};

struct FilterBank: public ActorBenchmark {
  const uint64_t simulations;
  const uint64_t columns;
  const uint64_t sinkrate;
  const uint64_t width;
  uint64_t channels;

  FilterBank(uint64_t columns, uint64_t simulations, uint64_t channels, uint64_t sinkrate):
    simulations(simulations), columns(columns), sinkrate(sinkrate), width(columns), channels(std::max((uint64_t)2, std::min((uint64_t)33, channels))) {}

  void run() {
    using namespace filterbank;

    vector<vector<uint64_t>> h(channels);
    vector<vector<uint64_t>> f(channels);

    for (uint64_t i = 0; i < channels; ++i) {
      h[i] = vector<uint64_t>(width);
      f[i] = vector<uint64_t>(width);
      for (uint64_t j = 0; j < width; ++j) {
        h[i][j] = (j * columns) + (j * channels) + i + j + i + 1;
        f[i][j] = (i * j) + (i * i) + i + j;
      }
    }

    auto producer = make_cown<Producer>(simulations);
    auto sink = make_cown<Sink>(sinkrate);
    auto combine = make_cown<Combine>(move(sink));
    auto integrator = make_cown<Integrator>(channels, move(combine));
    auto branch = make_cown<Branch>(channels, columns, h, f, move(integrator));
    auto source = make_cown<Source>(producer, move(branch));

    Producer::next(producer, move(source));
  }

  std::string name() { return "Filterbank"; }
};

};