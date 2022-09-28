#include <cpp/when.h>
#include <util/bench.h>
#include <util/random.h>
#include <cmath>
#include <unordered_map>
#include <variant>

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
        tie(l, p, r) = pivotize(move(input), pivot);

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

// namespace {
//   // lets do this badly
//   cown_ptr<vector<int>> map(vector<int> input, function<int(int)> f) {
//     if (input.size() == 0) {
//       return make_cown<vector<int>>(input);
//     } else if (input.size() <= 1) {
//       auto result = make_cown<vector<int>>();
//       when(result) << [input=move(input)](acquired_cown<vector<int>> result) {
//         result->push_back(input[0]);
//       };
//       return result;
//     } else {
//       // split async and join
//       when() << [input=move(input), f]() mutable {
//         auto split = input.size() / 2;
//         vector<int> right;
//         while(split-- > 0) {
//           right.insert(right.begin(), input.back());
//           input.pop_back();
//         }
//         auto left = move(input);

//         when(map(move(left), f), map(move(right), f)) << [](acquired_cown<vector<int>> left, acquired_cown<vector<int>> right) {
//           left->insert(left->end(), right->begin(), right->end());
//         };
//       };
//     }
//   }
// }

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
    cown_ptr<vector<uint64_t>> result = Sorter::sort(move(data), threshold);

    // when(result) << [dataset = dataset](acquired_cown<vector<uint64_t>> result) {
    //   assert(result->size() == dataset);
    //   assert(is_sorted(result->begin(), result->end()));
    // };
  }

  std::string name() { return "Quicksort"; }
};

};