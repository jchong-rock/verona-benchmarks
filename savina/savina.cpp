#include "util/bench.h"

#include "actors/benchmarks.h"
#include "boc/benchmarks.h"

#include <algorithm>

// FIXME: This is a quick hack around running just a single benchmark
// for profiling. Ideally the framework would be redesigned to enable this
// do you cook???

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
    savina.run<b>(__VA_ARGS__);

int main(const int argc, const char** argv) {
  BenchmarkHarness savina(argc, argv);

  if (savina.opt.has("--list"))
  {
    std::cout << "Benchmarks:\n"
      "\t" << actor_benchmark::Banking::name << "\n" <<
      "\t" << actor_benchmark::SleepingBarber::name << "\n" <<
      "\t" << actor_benchmark::BndBuffer::name << "\n" <<
      "\t" << actor_benchmark::Cigsmok::name << "\n" <<
      "\t" << actor_benchmark::Concdict::name << "\n" <<
      "\t" << actor_benchmark::DiningPhilosophers::name << "\n" <<
      "\t" << actor_benchmark::Logmap::name << "\n" <<
      "\t" << actor_benchmark::Concsll::name << "\n" <<
      "\t" << actor_benchmark::Big::name << "\n" <<
      "\t" << actor_benchmark::Chameneos::name << "\n" <<
      "\t" << actor_benchmark::Count::name << "\n" <<
      "\t" << actor_benchmark::Fib::name << "\n" <<
      "\t" << actor_benchmark::Fjcreate::name << "\n" <<
      "\t" << actor_benchmark::Fjthrput::name << "\n" <<
      "\t" << actor_benchmark::PingPong::name << "\n" <<
      "\t" << actor_benchmark::ThreadRing::name << "\n" <<
      "\t" << actor_benchmark::FilterBank::name << "\n" <<
      "\t" << actor_benchmark::Quicksort::name << "\n" <<
      "\t" << actor_benchmark::Radixsort::name << "\n" <<
      "\t" << actor_benchmark::Recmatmul::name << "\n" <<
      "\t" << actor_benchmark::Sieve::name << "\n" <<
      "\t" << actor_benchmark::Trapezoid::name << "\n";
    return 0;
  }

  std::string benchmark = savina.opt.is("--benchmark", "");

  if (savina.opt.has("--actor"))
  {
    // Actors
    RUN(actor_benchmark::Banking, 1000, 50000);
    RUN(actor_benchmark::SleepingBarber, 5000, 1000, 1000, 1000);
    RUN(actor_benchmark::BndBuffer, 50, 40, 40, 1000, 25, 25);
    RUN(actor_benchmark::Cigsmok, 1000, 200);
    RUN(actor_benchmark::Concdict, 20, 10000, 10);
    RUN(actor_benchmark::DiningPhilosophers, 20, 10000, 1);
    RUN(actor_benchmark::Logmap, 25000, 10, 3.64, 0.0025);
    RUN(actor_benchmark::Concsll, 20, 8000, 1, 10);

    RUN(actor_benchmark::Big, 20000, 120);
    RUN(actor_benchmark::Chameneos, 100, 200000);
    RUN(actor_benchmark::Count, 1000000);
    RUN(actor_benchmark::Fib, 25);
    RUN(actor_benchmark::Fjcreate, 40000);
    RUN(actor_benchmark::Fjthrput, 10000, 60, 1, true);
    RUN(actor_benchmark::PingPong, 40000);
    RUN(actor_benchmark::ThreadRing, 100, 100000);

    RUN(actor_benchmark::FilterBank, 16384, 34816, 8, 100);
    RUN(actor_benchmark::Quicksort, 1000000, uint64_t(1) << 60, 2048, 1024);
    RUN(actor_benchmark::Radixsort, 100000, uint64_t(1) << 60, 2048);
    RUN(actor_benchmark::Recmatmul, 20, 1024, 16384, 10);
    RUN(actor_benchmark::Sieve, 100000, 1000);
    RUN(actor_benchmark::Trapezoid, 10000000, 100, 1, 5);
  }

  if (savina.opt.has("--full"))
  {
    // BoC
    RUN(boc_benchmark::Banking, 1000, 50000);
    RUN(boc_benchmark::SleepingBarber, 5000, 1000, 1000, 1000);
    // RUN(boc_benchmark::BndBuffer, 50, 40, 40, 1000, 25, 25); - I don't think this one is a good translation
    RUN(boc_benchmark::Concdict, 20, 10000, 10);
    RUN(boc_benchmark::DiningPhilosophers, 20, 10000);
    RUN(boc_benchmark::Logmap, 25000, 10, 3.64, 0.0025);

    RUN(boc_benchmark::Chameneos, 100, 200000);
    RUN(boc_benchmark::Count, 1000000);
    RUN(boc_benchmark::Fib, 25);
    RUN(boc_benchmark::Fjcreate, 40000);
    RUN(boc_benchmark::Fjthrput, 10000, 60, 1, true);

    RUN(boc_benchmark::Quicksort, 1000000, uint64_t(1) << 60, 2048, 1024);
    RUN(boc_benchmark::Trapezoid, 10000000, 100, 1, 5);
  }

  if (savina.opt.has("--scale"))
  {
    // BoC
    RUN(boc_benchmark::Banking, 1000, 50000, true);
  }

  if (savina.opt.has("--fib"))
  {
    // BoC
    RUN(boc_benchmark::Fib, 20);
  }

  if (savina.opt.has("--leader"))
  {
    RUN(actor_benchmark::Leader<10>);
  }

  if (savina.opt.has("--leader2"))
  {
    RUN(actor_benchmark::Leader2, 100, 5);
  }

  if (savina.opt.has("--concdict"))
  {
    RUN(boc_benchmark::Concdict, 20, 10000, 10);
  }

  if (savina.opt.has("--dictler"))
  {
    RUN(boc_benchmark::Dictler);
  }

}