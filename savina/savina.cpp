#include "util/bench.h"

#include "actors/benchmarks.h"
#include "boc/benchmarks.h"

int main(const int argc, const char** argv) {
  BenchmarkHarness savina(argc, argv);

  savina.run<ActorBenchmark::Banking, 12>(1000, 50000);
  savina.run<ActorBenchmark::SleepingBarber, 12>(5000, 1000, 1000, 1000);
  savina.run<ActorBenchmark::BndBuffer, 12>(50, 40, 40, 1000, 25, 25);
  savina.run<ActorBenchmark::Cigsmok, 12>(1000, 200);
  savina.run<ActorBenchmark::DiningPhilosophers, 12>(20, 10000, 1);
  savina.run<ActorBenchmark::Fib, 12>(25);
  savina.run<ActorBenchmark::Concdict, 12>(20, 10000, 10);

  savina.run<BOCBenchmark::Banking, 12>(1000, 50000);
  savina.run<BOCBenchmark::SleepingBarber, 12>(5000, 1000, 1000, 1000);
  savina.run<BOCBenchmark::BndBuffer, 12>(50, 40, 40, 1000, 25, 25);
  savina.run<BOCBenchmark::DiningPhilosophers, 12>(20, 10000);
  savina.run<BOCBenchmark::Fib, 12>(25);
}