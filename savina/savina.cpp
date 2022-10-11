#include "util/bench.h"

#include "actors/benchmarks.h"
#include "boc/benchmarks.h"

int main(const int argc, const char** argv) {
  BenchmarkHarness savina(argc, argv);

  // Actors
  savina.run<actor_benchmark::Banking>(1000, 50000);
  // savina.run<actor_benchmark::SleepingBarber>(5000, 1000, 1000, 1000);
  // savina.run<actor_benchmark::BndBuffer>(50, 40, 40, 1000, 25, 25);
  // savina.run<actor_benchmark::Cigsmok>(1000, 200);
  // savina.run<actor_benchmark::Concdict>(20, 10000, 10);
  // savina.run<actor_benchmark::DiningPhilosophers>(20, 10000, 1);
  // savina.run<actor_benchmark::Logmap>(25000, 10, 3.64, 0.0025);
  // savina.run<actor_benchmark::Concsll>(20, 8000, 1, 10);

  // savina.run<actor_benchmark::Big>(20000, 120);
  // savina.run<actor_benchmark::Chameneos>(100, 200000);
  // savina.run<actor_benchmark::Count>(1000000);
  // savina.run<actor_benchmark::Fib>(25);
  // savina.run<actor_benchmark::Fjcreate>(40000);
  // savina.run<actor_benchmark::Fjthrput>(10000, 60, 1, true);
  // savina.run<actor_benchmark::PingPong>(40000);
  // savina.run<actor_benchmark::ThreadRing>(100, 100000);

  // savina.run<actor_benchmark::FilterBank>(16384, 34816, 8, 100);
  // savina.run<actor_benchmark::Quicksort>(1000000, uint64_t(1) << 60, 2048, 1024);
  // savina.run<actor_benchmark::Radixsort>(100000, uint64_t(1) << 60, 2048);
  // savina.run<actor_benchmark::Recmatmul>(20, 1024, 16384, 10);
  // savina.run<actor_benchmark::Sieve>(100000, 1000);
  // savina.run<actor_benchmark::Trapezoid>(10000000, 100, 1, 5);

  // BoC
  // std::cout << "boc" << std::endl;
  // savina.run<boc_benchmark::Banking>(1000, 50000);
  // savina.run<boc_benchmark::SleepingBarber>(5000, 1000, 1000, 1000);
  // savina.run<boc_benchmark::BndBuffer>(50, 40, 40, 1000, 25, 25);
  // savina.run<boc_benchmark::Concdict>(20, 10000, 10);
  // savina.run<boc_benchmark::DiningPhilosophers>(20, 10000);
  // savina.run<boc_benchmark::Logmap>(25000, 10, 3.64, 0.0025);

  // savina.run<boc_benchmark::Chameneos>(100, 200000);
  // savina.run<boc_benchmark::Count>(1000000);
  // savina.run<boc_benchmark::Fib>(25);
  // savina.run<boc_benchmark::Fjcreate>(40000);
  // savina.run<boc_benchmark::Fjthrput>(10000, 60, 1, true);

  // savina.run<boc_benchmark::Quicksort>(1000000, uint64_t(1) << 60, 2048, 1024);
  // savina.run<boc_benchmark::Trapezoid>(10000000, 100, 1, 5);

}