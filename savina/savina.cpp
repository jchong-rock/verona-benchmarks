#include "util/bench.h"

#include "actors/benchmarks.h"
#include "boc/benchmarks.h"

int main(const int argc, const char** argv) {
  BenchmarkHarness savina(argc, argv);

  // Actors
  savina.run<actor_benchmark::Banking, 12>(1000, 50000);
  savina.run<actor_benchmark::SleepingBarber, 12>(5000, 1000, 1000, 1000);
  savina.run<actor_benchmark::BndBuffer, 12>(50, 40, 40, 1000, 25, 25);
  savina.run<actor_benchmark::Cigsmok, 12>(1000, 200);
  savina.run<actor_benchmark::Concdict, 12>(20, 10000, 10);
  savina.run<actor_benchmark::DiningPhilosophers, 12>(20, 10000, 1);
  savina.run<actor_benchmark::Logmap, 12>(25000, 10, 3.64, 0.0025);
  savina.run<actor_benchmark::Concsll, 12>(20, 8000, 1, 10);

  savina.run<actor_benchmark::Big, 12>(20000, 120);
  savina.run<actor_benchmark::Chameneos, 12>(100, 200000);
  savina.run<actor_benchmark::Count, 12>(1000000);
  savina.run<actor_benchmark::Fib, 12>(25);
  savina.run<actor_benchmark::Fjcreate, 12>(40000);
  savina.run<actor_benchmark::Fjthrput, 12>(10000, 60, 1, true);
  savina.run<actor_benchmark::PingPong, 12>(40000);
  savina.run<actor_benchmark::ThreadRing, 12>(100, 100000);

  savina.run<actor_benchmark::FilterBank, 12>(16384, 34816, 8, 100);
  savina.run<actor_benchmark::Quicksort, 12>(1000000, uint64_t(1) << 60, 2048, 1024);

  // BoC
  savina.run<boc_benchmark::Banking, 12>(1000, 50000);
  savina.run<boc_benchmark::SleepingBarber, 12>(5000, 1000, 1000, 1000);
  savina.run<boc_benchmark::BndBuffer, 12>(50, 40, 40, 1000, 25, 25);
  savina.run<boc_benchmark::Concdict, 12>(20, 10000, 10);
  savina.run<boc_benchmark::DiningPhilosophers, 12>(20, 10000);
  savina.run<boc_benchmark::Logmap, 12>(25000, 10, 3.64, 0.0025);

  savina.run<boc_benchmark::Chameneos, 12>(100, 200000);
  savina.run<boc_benchmark::Count, 12>(1000000);
  savina.run<boc_benchmark::Fib, 12>(25);

  savina.run<boc_benchmark::Quicksort, 12>(1000000, uint64_t(1) << 60, 2048, 1024);

  // Read percentage experiments
  // savina.run<actor_benchmark::Concdict, 12>(20, 10000, 0);
  // savina.run<actor_benchmark::Concdict, 12>(20, 10000, 10);
  // savina.run<actor_benchmark::Concdict, 12>(20, 10000, 900);

  // savina.run<boc_benchmark::Concdict, 12>(20, 10000, 0);
  // savina.run<boc_benchmark::Concdict, 12>(20, 10000, 10);
  // savina.run<boc_benchmark::Concdict, 12>(20, 10000, 900);

}