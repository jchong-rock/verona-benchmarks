#pragma once

#include <cpp/when.h>
#include <test/harness.h>
#include <float.h>
#include "stats.h"

using verona::cpp::make_cown;
using verona::cpp::cown_ptr;
using verona::cpp::acquired_cown;

struct Bench {
  opt::Opt opt;

  size_t cores;
  bool detect_leaks;

  Bench(const int argc, const char** argv) : opt(argc, argv)
  {
    std::cout << "Bench starting." << std::endl;

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

  template<typename... Args>
  void run(void f(Args...), Args... args)
  {
    SampleStats samples;

    size_t iterations = 1;

    for (size_t i = 0; i < iterations; ++i) {
      Scheduler& sched = Scheduler::get();

      sched.init(cores);

      high_resolution_clock::time_point start = high_resolution_clock::now();

      f(std::forward<Args>(args)...);

      sched.run();

      samples.add(duration_cast<milliseconds>((high_resolution_clock::now() - start)).count());

      if (detect_leaks)
        snmalloc::debug_check_empty<snmalloc::Alloc::Config>();
    }

    std::cout << "name" << "   "
              << samples.mean() << " ms   "
              << samples.median() << " ms   "
              << "+/- " << samples.ref_err() << " %   "
              << samples.stddev()
              << std::endl;
  }
};