#include <cpp/when.h>
#include <util/bench.h>
#include <util/random.h>
#include <cmath>
#include <unordered_map>

namespace boc_benchmark {

namespace quicksort {

using namespace std;

namespace Sorter {
  tuple<vector<uint64_t>, vector<uint64_t>, vector<uint64_t>> pivotize(vector<uint64_t> input, uint64_t pivot) {
    vector<uint64_t> l;
    vector<uint64_t> p;
    vector<uint64_t> r;

    for (auto item: input) {
      if (item < pivot)
        l.push_back(item);
      else if (item > pivot)
        r.push_back(item);
      else
        p.push_back(item);
    }

    return make_tuple(move(l), move(p), move(r));
  }

  vector<uint64_t> sort_sequentially(vector<uint64_t> input) {
    uint64_t size = input.size();

    if (size < 2)
      return input;

    uint64_t pivot = input[size / 2];
    vector<uint64_t> l;
    vector<uint64_t> p;
    vector<uint64_t> r;
    tie(l, p, r) = pivotize(move(input), pivot);

    l = sort_sequentially(move(l));
    r = sort_sequentially(move(r));

    vector<uint64_t> sorted;
    sorted.insert(sorted.end(), l.begin(), l.end());
    sorted.insert(sorted.end(), p.begin(), p.end());
    sorted.insert(sorted.end(), r.begin(), r.end());

    return sorted;
  }

  // this is doing a lot of the work in one thread and only deferring to do the final sorts and the concat.
  // but it still seems to be the faster version
  cown_ptr<vector<uint64_t>> sort(vector<uint64_t> input, const uint64_t threshold) {
    uint64_t size = input.size();

    if (size < threshold){
      cown_ptr<vector<uint64_t>> result = make_cown<vector<uint64_t>>();
      when(result) << [input=move(input)](acquired_cown<vector<uint64_t>> result) {
        *result = sort_sequentially(move(input));
      };
      return result;
    } else {
      uint64_t pivot = input[size / 2];

      vector<uint64_t> l;
      vector<uint64_t> p;
      vector<uint64_t> r;
      tie(l, p, r) = move(pivotize(move(input), pivot));

      auto left = Sorter::sort(move(l), threshold);
      auto right = Sorter::sort(move(r), threshold);
      when(left, right) << [p=move(p)] (acquired_cown<vector<uint64_t>> l, acquired_cown<vector<uint64_t>> r) {
        l->insert(l->end(), p.begin(), p.end());
        l->insert(l->end(), r->begin(), r->end());
      };

      return left;
    }
  }
};

};

struct Quicksort: public BocBenchmark {
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
    cown_ptr<vector<uint64_t>> result = move(Sorter::sort(move(data), threshold));
    // when(result) << [dataset = dataset](acquired_cown<vector<uint64_t>> result) {
    //   assert(result->size() == dataset);
    //   assert(is_sorted(result->begin(), result->end()));
    // };
  }

  inline static const std::string name = "Quicksort";
};

};