#include <cpp/when.h>
#include <util/bench.h>
#include <random>
#include <cmath>
#include <unordered_map>

namespace actor_benchmark {
namespace {

using namespace verona::cpp;
using namespace std;

using Matrix = vector<vector<uint64_t>>;

struct Source {
  static void boot(cown_ptr<Source>);
};

struct Producer {
  const uint64_t simulations;
  uint64_t sent;

  Producer(uint64_t simulations): simulations(simulations), sent(0) {}

  static void next(cown_ptr<Producer> self, cown_ptr<Source> source) {
    when(self) << [source](acquired_cown<Producer> self) {
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

  static void value(cown_ptr<Sink> self, uint64_t n) {
    when(self) << [n](acquired_cown<Sink> self) {
      self->count = (self->count + 1) % self->sinkrate;
    };
  }
};

struct Combine {
  cown_ptr<Sink> sink;

  Combine(cown_ptr<Sink> sink): sink(sink) {}

  static void collect(cown_ptr<Combine> self, unique_ptr<unordered_map<uint64_t, uint64_t>> map) {
    when(self) << [map=std::move(map)](acquired_cown<Combine> self) {
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

  Integrator(uint64_t channels, cown_ptr<Combine> combine): channels(channels), combine(combine) {}

  static void value(cown_ptr<Integrator> self, uint64_t id, uint64_t n) {
    when(self) << [id, n](acquired_cown<Integrator> self) mutable{
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

// trying to beat obesity by loosening your belt

struct Delay {
  const uint64_t length;
  cown_ptr<FirFilter> filter;
  vector<uint64_t> state;
  uint64_t placeholder;

  Delay(uint64_t length, cown_ptr<FirFilter> filter): length(length), filter(filter), state(length, 0), placeholder(0) {}

  static void value(cown_ptr<Delay> self, uint64_t n) {
    when(self) << [n](acquired_cown<Delay> self) {
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

  SampleFilter(uint64_t rate, cown_ptr<Delay> delay): rate(rate), delay(delay), samples_received(0) {}

  static void value(cown_ptr<SampleFilter> self, uint64_t n) {
    when(self) << [n](acquired_cown<SampleFilter> self) {
      if (self->samples_received == 0) {
        Delay::value(n);
      } else {
        Delay::value(0);
      }

      self->samples_received = (self->samples_received + 1) % self->rate;
    };
  }
};

// actor TaggedForward
//   let _id: U64
//   let _integrator: Integrator

//   new create(id: U64, integrator: Integrator) =>
//     _id = id
//     _integrator = integrator

//   be value(n: U64) =>
//     _integrator.value(_id, n)

struct Bank {
  cown_ptr<Delay> entry;

  Bank(uint64_t id, uint64_t columns, vector<uint64_t> h, vector<uint64_t> f, cown_ptr<Integrator>): entry(make_cown<Delay>(columns - 1)) {

  }

//   new create(id: U64, columns: U64, h: Array[U64] val, f: Array[U64] val, integrator: Integrator) =>
//     _entry = Delay(columns - 1, 
//       FirFilter(columns, h, 
//         SampleFilter(columns, 
//           Delay(columns - 1, 
//             FirFilter(columns, f, 
//               TaggedForward(id, integrator))))))  
  
//   be value(n: U64) =>
//     _entry.value(n)

};

struct Branch {
  uint64_t channels;
  uint64_t columns;
  Matrix h;
  Matrix f;
  cown_ptr<Integrator> integrator;
  vector<cown_ptr<Bank>> banks;
};

// actor Branch
//   let _channels: U64
//   let _columns: U64
//   let _h: Matrix
//   let _f: Matrix
//   let _integrator: Integrator
//   let _banks: Array[Bank]

//   new create(channels: U64, columns: U64, h: Matrix, f: Matrix, integrator: Integrator) =>
//     _channels = channels
//     _columns = columns
//     _h = h
//     _f = f
//     _integrator = integrator

//     _banks = Array[Bank]

//     var index: USize = 0

//     for i in Range[U64](0, _channels) do
//       index = i.usize()

//       try
//         _banks.push(Bank(i, _columns, _h(index)?, _f(index)?, _integrator))
//       end
//     end

//   be value(n: U64) =>
//     for bank in _banks.values() do
//       bank.value(n)
//     end

// actor Source
//   let _producer: Producer
//   let _branch: Branch
//   let _max: U64
//   var _current: U64

//   new create(producer: Producer, branch: Branch) =>
//     _producer = producer
//     _branch = branch
//     _max = 1000
//     _current = 0

//   be boot() =>
//     _branch.value(_current)
//     _current = (_current + 1) % _max
//     _producer.next(this)



// actor FirFilter
//   let _length: U64
//   let _coefficients: Array[U64] val
//   let _next: (SampleFilter | TaggedForward)

//   var _data: Array[U64]
//   var _index: USize
//   var _is_full: Bool

//   new create(length: U64, coefficients: Array[U64] val, next: (SampleFilter | TaggedForward)) =>
//     _length = length
//     _coefficients = coefficients
//     _next = next

//     _data = Array[U64].init(U64(0), _length.usize())
//     _index = 0
//     _is_full = false

//   be value(n: U64) =>
//     try
//       _data(_index)? = n
//       _index = _index + 1

//       if _index == _length.usize() then
//         _is_full = true
//         _index = 0
//       end
      
//       if _is_full then
//         var sum: U64 = 0
//         var i: USize = 0

//         while i < _length.usize() do
//           sum = sum + (_data(i)? * _coefficients(_length.usize() - i - 1)?)
//           i = i + 1
//         end
      
//         _next.value(sum)
//       end
//     end


struct FilterBank: public AsyncBenchmark {
  const uint64_t simulations;
  const uint64_t columns;
  const uint64_t sinkrate;
  const uint64_t width;
  uint64_t channels;

  FilterBank(uint64_t columns, uint64_t simulations, uint64_t channels, uint64_t sinkrate):
    simulations(simulations), columns(columns), sinkrate(sinkrate), width(columns), channels(max((uint64_t)2, min((uint64_t)33, channels))) {}

  void run() {
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

    cown_ptr<Producer> producer = make_cown<Producer>(simulations);
    cown_ptr<Sink> sink = make_cown<Sink>(sinkrate);
    cown_ptr<Combine> combine = make_cown<Combine>(sink);
    cown_ptr<Integrator> Integrator = make_cown<Integrator>(channels, combine);
//     let combine = Combine(sink)
//     let integrator = Integrator(_channels, combine)
//     let branch = Branch(_channels, _columns, consume h, consume f, integrator)
//     let source = Source(producer, branch)

//     producer.next(source)
  }

  std::string name() { return "FilterBank"; }
};

};

};