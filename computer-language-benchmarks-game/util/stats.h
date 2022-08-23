#pragma once

#include <set>
#include <cmath>

struct SampleStats {
  std::vector<double> samples;
  bool sorted = true;

  SampleStats() {};

  void add(double sample) {
    samples.push_back(sample);
    sorted = false;
  }

  double sum() {
    double total = 0;
    for (const double& sample: samples) {
      total += sample;
    }

    return total;
  }

  double mean() { return sum() / samples.size(); }

  double median() {
    if (std::exchange(sorted, true)) std::sort(samples.begin(), samples.end());

    uint64_t size = samples.size();

    if (size == 0) {
      return 0;
    } else if (size == 1) {
      return samples[0];
    } else {
      uint64_t middle = size / 2;

      if (size % 2 == 1) {
        return samples[middle];
      } else {
        return (samples[middle - 1] + samples[middle]) / 2.0;
      }
    }
  }

  double geometric_mean() {
    double result = 0;

    for (const double& sample: samples) {
      result += std::log10(sample);
    }

    return std::pow(result / ((double)samples.size()), 10);
  }

  double harmonic_mean() {
    double denom = 0;

    for (const double& sample: samples) {
      denom += (1.0 / sample);
    }

    return ((double) samples.size()) / denom;
  }

  double stddev() {
    double m = mean();
    double temp = 0;

    for (const double& sample: samples) {
      temp += ((m - sample) * (m - sample));
    }

    return std::sqrt(temp / ((double)samples.size()));
  }

  double ref_err() { return 100.0 * ((confidence_high() - mean()) / mean()); }

  double variation() { return stddev() / mean(); }

  double confidence_low() { return mean() - (1.96 * (stddev() / std::sqrt(samples.size()))); }

  double confidence_high() { return mean() + (1.96 * (stddev() / std::sqrt(samples.size()))); }

  double skewness() {
    double m = mean();
    double sd = stddev();
    double total = 0;
    double diff = 0;

    if (samples.size() > 0) {
      for (const double& sample: samples) {
        diff = sample - m;
        total += (diff * diff * diff);
      }

      return total / ((((double) samples.size()) - 1) * sd * sd * sd);
    } else {
      return 0;
    }
  }
};