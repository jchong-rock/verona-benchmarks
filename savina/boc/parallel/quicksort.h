#include <cpp/when.h>
#include <util/bench.h>
#include <util/random.h>
#include <cmath>
#include <unordered_map>
#include <variant>

namespace boc_benchmark {

namespace quicksort {

using namespace std;

enum class Position {
  Initial, Left, Right
};

enum class None {};

struct Sorter {
  variant<cown_ptr<Sorter>, None> parent;
  Position position;
  uint64_t threshold;
  uint64_t length;
  uint64_t fragments;
  variant<vector<uint64_t>, None> results; // or None?

  Sorter(variant<cown_ptr<Sorter>, None> parent, Position position, uint64_t threshold, uint64_t length):
    parent(parent), position(position), threshold(threshold), length(length), fragments(0), results(None()) {}

  tuple<vector<uint64_t>, vector<uint64_t>, vector<uint64_t>> pivotize(vector<uint64_t> input, uint64_t pivot) {
    vector<uint64_t> l;
    vector<uint64_t> r;
    vector<uint64_t> p;

    for (auto item: input) {
      if (item < pivot)
        l.push_back(item);
      else if (item > pivot)
        r.push_back(item);
      else
        p.push_back(item);
    }

      return make_tuple(l, p, r);
  }

  vector<uint64_t> sort_sequentially(vector<uint64_t> input) {
    uint64_t size = input.size();

    if (size < 2)
      return input;

    uint64_t pivot = input[size / 2];
    vector<uint64_t> l;
    vector<uint64_t> p;
    vector<uint64_t> r;
    tie(l, p, r) = pivotize(input, pivot);

    vector<uint64_t> sorted;
    sorted.insert(sorted.end(), l.begin(), l.end());
    sorted.insert(sorted.end(), p.begin(), p.end());
    sorted.insert(sorted.end(), r.begin(), r.end());

    return sorted;
  }

  void notify_parent() {
    if (position == Position::Initial) {
      /* done */
    } else {
      try {
        Sorter::result(get<cown_ptr<Sorter>>(parent), get<vector<uint64_t>>(results), position);
      } catch (const bad_variant_access& ex) {
          // this seems to not be expected in the original benchmark
          std::cout << ex.what() << '\n';
          throw(ex);
      }
    }
  }

  static void sort(cown_ptr<Sorter> self, vector<uint64_t> input) {
    when(self) << [tag=self, input=move(input)](acquired_cown<Sorter> self) {
      uint64_t size = input.size();

      if (size < self->threshold){
        self->results = self->sort_sequentially(input);
        self->notify_parent();
      } else {
        uint64_t pivot = input[size / 2];

        vector<uint64_t> l;
        vector<uint64_t> p;
        vector<uint64_t> r;
        tie(l, p, r) = self->pivotize(input, pivot);

        Sorter::sort(make_cown<Sorter>(tag, Position::Left, self->threshold, self->length), l);
        Sorter::sort(make_cown<Sorter>(tag, Position::Right, self->threshold, self->length), r);

        self->results = p;
        self->fragments++;
      }
    };
  }

  static void result(cown_ptr<Sorter> self, vector<uint64_t> sorted, Position position) {
    when(self) << [tag=self, sorted, position](acquired_cown<Sorter> self) {
      if (sorted.size() > 0) {
        vector<uint64_t> temp;

        visit(overloaded {
          [&](None) { self->results = None(); },
          [&](vector<uint64_t> data) {
            if (self->position == Position::Left) {
              temp.insert(temp.end(), sorted.begin(), sorted.end());
              temp.insert(temp.end(), data.begin(), data.end());
            } else if (self->position == Position::Right) {
              temp.insert(temp.end(), data.begin(), data.end());
              temp.insert(temp.end(), sorted.begin(), sorted.end());
            }
          }
        }, self->results);
      }

      self->fragments++;

      if (self->fragments == 3)
        self->notify_parent();
    };
  }
};

};

struct Quicksort: public AsyncBenchmark {
  uint64_t dataset;
  uint64_t max;
  uint64_t threshold;
  uint64_t seed;

  Quicksort(uint64_t dataset, uint64_t max, uint64_t threshold, uint64_t seed):
    dataset(dataset), max(max), threshold(threshold), seed(seed) {}


  void run() {
    using namespace std;

    vector<uint64_t> data;
    SimpleRand random(seed);

    for (uint64_t i = 0; i < dataset; ++i) {
      data.push_back(random.nextLong() % max);
    }

    using namespace quicksort;
    Sorter::sort(make_cown<Sorter>(None(), Position::Initial, threshold, dataset), data);
  }

  std::string name() { return "Quicksort"; }
};

};