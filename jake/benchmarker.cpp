#include "util/bench.h"

#include "examples/leader_ring.h"
#include "examples/leader_ring_boc.h"
#include "examples/leader_tree.h"
#include "examples/leader_arbitrary.h"
#include "examples/breakfast.h"
#include "examples/breakfast_ideal.h"
#include "examples/timed/timed.h"

#include <algorithm>

// Copied from Savina benchmarking suite.
// FIXME: This is a quick hack around running just a single benchmark
// for profiling. Ideally the framework would be redesigned to enable this

static bool iequals(const std::string& a, const std::string& b)
{
    return std::equal(a.begin(), a.end(),
                      b.begin(), b.end(),
                      [](char a, char b) {
                          return tolower(a) == tolower(b);
                      });
}

#define RUN(b, ...) \
  if (benchmark.empty() || iequals(benchmark, b::name)) \
    benchmarker.run<b>(__VA_ARGS__);

int main(const int argc, const char** argv) {
  BenchmarkHarness benchmarker(argc, argv);

  std::string benchmark = benchmarker.opt.is("--benchmark", "");

  size_t servers = benchmarker.opt.is<size_t>("--servers", 100);
  size_t divisions = benchmarker.opt.is<size_t>("--divisions", 5);

  servers = benchmarker.opt.is<size_t>("--bacon", servers);
  divisions = benchmarker.opt.is<size_t>("--eggs", divisions);

  if (benchmarker.opt.has("--leader_ring")) 
    RUN(jake_benchmark::LeaderRing, servers, divisions);
  
  if (benchmarker.opt.has("--leader_ring_boc")) 
    RUN(jake_benchmark::LeaderRingBoC, servers, divisions);

  if (benchmarker.opt.has("--leader_tree"))
    RUN(jake_benchmark::LeaderTree, servers, divisions);

  if (benchmarker.opt.has("--leader_arbitrary"))
    RUN(jake_benchmark::LeaderArbitrary, servers, divisions);

  if (benchmarker.opt.has("--breakfast"))
    RUN(jake_benchmark::Breakfast);

  if (benchmarker.opt.has("--breakfast_ideal"))
    RUN(jake_benchmark::BreakfastIdeal, servers, divisions);

  if (benchmarker.opt.has("--timed"))
    RUN(jake_benchmark::TimedBench, servers, divisions);

}