#include "util/bench.h"

#include "examples/leader.h"
#include "examples/leader_multiple_starts.h"
#include "examples/leader_dag.h"
#include "examples/leader_dag_broken.h"
#include "examples/leader_dag_no_mailbox.h"
#include "examples/leader_arbitrary.h"
#include "examples/breakfast.h"

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

  if (benchmarker.opt.has("--leader")) {
    RUN(jake_benchmark::Leader, servers);
  }
  else {
    size_t divisions = benchmarker.opt.is<size_t>("--divisions", 5);

    if (benchmarker.opt.has("--leader_multiple_starts"))
      RUN(jake_benchmark::LeaderMultiStart, servers, divisions);
    

    if (benchmarker.opt.has("--leader_dag"))
      RUN(jake_benchmark::LeaderDAG, servers, divisions);
    

    if (benchmarker.opt.has("--leader_dag_broken"))
      RUN(jake_benchmark::LeaderDAGBroken, servers, divisions);
    

    if (benchmarker.opt.has("--leader_dag_no_mailbox"))
      RUN(jake_benchmark::LeaderDAGNoMailbox, servers, divisions);

    if (benchmarker.opt.has("--leader_arbitrary"))
      RUN(jake_benchmark::LeaderArbitrary, servers, divisions);

    if (benchmarker.opt.has("--breakfast"))
      RUN(jake_benchmark::Breakfast, servers, divisions);
  }
}