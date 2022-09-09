#pragma once

#include <cpp/when.h>
#include <test/harness.h>
#include <float.h>
#include "stats.h"

using verona::cpp::make_cown;
using verona::cpp::cown_ptr;
using verona::cpp::acquired_cown;

struct AsyncBenchmark {
  virtual void run()=0;
  virtual std::string name()=0;
  virtual ~AsyncBenchmark() {}
};

struct Writer {
  virtual void writeHeader()=0;
  virtual void writeEntry(std::string benchmark, double mean, double median, double error, double stddev)=0;
};

struct CSVWriter: public Writer {
  void writeHeader() override {
    std::cout << "benchmark,mean,median,error,stddev" << std::endl;
  }

  void writeEntry(std::string benchmark, double mean, double median, double error, double stddev) override {
    std::cout << benchmark << "," << mean << "," << median << "," << error << "," << stddev << std::endl;
  }
};

struct ConsoleWriter: public Writer {
  void writeHeader() { }

  void writeEntry(std::string benchmark, double mean, double median, double error, double stddev) override {
    std::cout << benchmark << "   "
              << mean << " ms   "
              << median << " ms   "
              << "+/- " << error << " %   "
              << stddev
              << std::endl;
  }
};

struct BenchmarkHarness {
  opt::Opt opt;

  size_t cores;
  bool detect_leaks;
  std::unique_ptr<Writer> writer;

  BenchmarkHarness(const int argc, const char** argv) : opt(argc, argv) {
    std::cout << "BenchmarkHarness starting." << std::endl;

    for (int i = 0; i < argc; i++)
    {
      std::cout << " " << argv[i];
    }

    size_t count = opt.is<size_t>("--seed_count", 1);

    std::cout << std::endl;

    cores = opt.is<size_t>("--cores", 4);

    detect_leaks = !opt.has("--allow_leaks");
    Scheduler::set_detect_leaks(detect_leaks);

    writer = opt.has("--csv") ? std::unique_ptr<Writer>{std::make_unique<CSVWriter>()} : std::make_unique<ConsoleWriter>();

    writer->writeHeader();
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

    writer->writeEntry(benchmark.name(), samples.mean(), samples.median(), samples.ref_err(), samples.stddev());
  }
};