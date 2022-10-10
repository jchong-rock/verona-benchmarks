#include <cpp/when.h>
#include <util/bench.h>
#include <util/random.h>
#include <cmath>
#include <unordered_map>

namespace actor_benchmark {

namespace quicksort {

using namespace std;

enum class Position {
  Initial, Left, Right
};

enum class None {};

struct Sorter {
  cown_ptr<Sorter> parent;
  Position position;
  uint64_t threshold;
  uint64_t length;
  uint64_t fragments;
  unique_ptr<vector<uint64_t>> results; // or None?

  Sorter(Position position, uint64_t threshold, uint64_t length):
    position(position), threshold(threshold), length(length), fragments(0), results(nullptr) {}

  Sorter(const cown_ptr<Sorter>& parent, Position position, uint64_t threshold, uint64_t length):
    parent(parent), position(position), threshold(threshold), length(length), fragments(0), results(nullptr) {}

  tuple<unique_ptr<vector<uint64_t>>, unique_ptr<vector<uint64_t>>, unique_ptr<vector<uint64_t>>> pivotize(unique_ptr<vector<uint64_t>> input, uint64_t pivot) {
    unique_ptr<vector<uint64_t>> l = make_unique<vector<uint64_t>>();
    unique_ptr<vector<uint64_t>> p = make_unique<vector<uint64_t>>();
    unique_ptr<vector<uint64_t>> r = make_unique<vector<uint64_t>>();

    for (auto item: *input) {
      if (item < pivot)
        l->push_back(item);
      else if (item > pivot)
        r->push_back(item);
      else
        p->push_back(item);
    }

    return make_tuple(move(l), move(p), move(r));
  }

  unique_ptr<vector<uint64_t>> sort_sequentially(unique_ptr<vector<uint64_t>> input) {
    uint64_t size = input->size();

    if (size < 2)
      return input;

    uint64_t pivot = input->at(size / 2);
    unique_ptr<vector<uint64_t>> l;
    unique_ptr<vector<uint64_t>> p;
    unique_ptr<vector<uint64_t>> r;
    tie(l, p, r) = pivotize(move(input), pivot);

    l = sort_sequentially(move(l));
    r = sort_sequentially(move(r));

    unique_ptr<vector<uint64_t>> sorted = make_unique<vector<uint64_t>>();
    sorted->insert(sorted->end(), l->begin(), l->end());
    sorted->insert(sorted->end(), p->begin(), p->end());
    sorted->insert(sorted->end(), r->begin(), r->end());

    return sorted;
  }

  void notify_parent() {
    if (position == Position::Initial) {
      // assert(is_sorted(results->begin(), results->end()));
      /* done */
    } else {
      Sorter::result(parent, move(results), position);
    }
  }

  static void sort(const cown_ptr<Sorter>& self, unique_ptr<vector<uint64_t>> input) {
    when(self) << [tag=self, input=move(input)](acquired_cown<Sorter> self) mutable {
      uint64_t size = input->size();

      if (size < self->threshold){
        self->results = self->sort_sequentially(move(input));
        self->notify_parent();
      } else {
        uint64_t pivot = input->at(size / 2);

        unique_ptr<vector<uint64_t>> l;
        unique_ptr<vector<uint64_t>> p;
        unique_ptr<vector<uint64_t>> r;
        tie(l, p, r) = self->pivotize(move(input), pivot);

        Sorter::sort(make_cown<Sorter>(tag, Position::Left, self->threshold, self->length), move(l));
        Sorter::sort(make_cown<Sorter>(tag, Position::Right, self->threshold, self->length), move(r));

        self->results = move(p);
        self->fragments++;
      }
    };
  }

  static void result(const cown_ptr<Sorter>& self, unique_ptr<vector<uint64_t>> sorted, Position position) {
    when(self) << [tag=self, sorted=move(sorted), position](acquired_cown<Sorter> self) mutable {
      if (sorted->size() > 0) {
        unique_ptr<vector<uint64_t>> temp = make_unique<vector<uint64_t>>();

        if (self->results != nullptr) {
          if (position == Position::Left) {
            temp->insert(temp->end(), sorted->begin(), sorted->end());
            temp->insert(temp->end(), self->results->begin(), self->results->end());
          } else if (position == Position::Right) {
            temp->insert(temp->end(), self->results->begin(), self->results->end());
            temp->insert(temp->end(), sorted->begin(), sorted->end());
          }
          self->results = move(temp);
        }

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

    unique_ptr<vector<uint64_t>> data = make_unique<vector<uint64_t>>();
    SimpleRand random(seed);

    for (uint64_t i = 0; i < dataset; ++i) {
      data->push_back(random.nextLong() % max);
    }

    // cout << "unsorted: ";
    // for(const auto& e: data)
    //   cout << e << ", ";
    // cout << endl;

    using namespace quicksort;
    Sorter::sort(make_cown<Sorter>(Position::Initial, threshold, dataset), move(data));
  }

  std::string name() { return "Quicksort"; }
};

};