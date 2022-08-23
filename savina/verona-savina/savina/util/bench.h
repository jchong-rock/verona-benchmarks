#pragma once

#include <cpp/when.h>
#include <test/harness.h>
#include <float.h>
#include "stats.h"

using verona::cpp::make_cown;
using verona::cpp::cown_ptr;
using verona::cpp::acquired_cown;

struct Savina;

struct AsyncBenchmark {
  virtual void run()=0;
  virtual std::string name()=0;
  virtual ~AsyncBenchmark() {}
};

struct Savina {
  opt::Opt opt;

  size_t cores;
  bool detect_leaks;

  Savina(const int argc, const char** argv) : opt(argc, argv) {
    std::cout << "Savina starting." << std::endl;

    for (int i = 0; i < argc; i++)
    {
      std::cout << " " << argv[i];
    }

    size_t count = opt.is<size_t>("--seed_count", 1);

    std::cout << std::endl;

    cores = opt.is<size_t>("--cores", 4);

    detect_leaks = !opt.has("--allow_leaks");
    Scheduler::set_detect_leaks(detect_leaks);
  }

  template<typename T, size_t iterations, typename...Args>
  void run(Args&&... args) {
    SampleStats samples;

    T benchmark(std::forward<Args>(args)...);

    for (size_t i = 0; i < iterations; ++i) {
      Scheduler& sched = Scheduler::get();

      sched.init(cores);

      high_resolution_clock::time_point start = high_resolution_clock::now();

      benchmark.run();

      sched.run();

      samples.add(duration_cast<milliseconds>((high_resolution_clock::now() - start)).count());

      if (detect_leaks)
        snmalloc::debug_check_empty<snmalloc::Alloc::Config>();
    }

    std::cout << benchmark.name() << "   "
              << samples.mean() << " ms   "
              << samples.median() << " ms   "
              << "+/- " << samples.ref_err() << " %   "
              << samples.stddev()
              << std::endl;
  }
};