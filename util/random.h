#pragma once

#include <utility>

// TODO: a lot of these benchmarks involve random durations so they will need to be the same to ensure the same busy wait magnitudes

// These conversions are not correct or at least comparable with Pony, which will make this
// hard to compare
struct Random {
  virtual uint64_t next() = 0;

  double real() { return ((double)(next() >> 11)) * (((double)1) / 9007199254740992); }

  // random int in [0, n)
  uint64_t integer(uint64_t n) { return ((uint64_t)real()) % n; }
};

struct SimpleRand : public Random {
  uint64_t value;

  SimpleRand(uint64_t x): value(x) {}

  uint64_t next() { return nextLong(); }

  uint64_t nextLong() { return std::exchange(value, ((value * 1309) + 13849) & 65535); }

  uint32_t nextInt() { return nextInt(0); }

  uint32_t nextInt(uint32_t max) { return max == 0 ? uint32_t(nextLong()) : (uint32_t(nextLong()) % max); }

  double nextDouble() { return double(1.0 / (nextLong() + 1)); }
};

struct XorOshiro128Plus : public Random {
// This is an implementation of xoroshiro128+, as detailed at:
// http://xoroshiro.di.unimi.it
// This is currently the default Rand implementation in Pony so using it for this util.
  uint64_t x;
  uint64_t y;

  XorOshiro128Plus(): XorOshiro128Plus(5489, 0) {}

  XorOshiro128Plus(uint64_t x): XorOshiro128Plus(x, 0) {}

  XorOshiro128Plus(uint64_t x, uint64_t y): x(x), y(y) {}

  uint64_t next() {
    // A random integer in [0, 2^64)
    const uint64_t _x = x;
    uint64_t _y = y;
    const uint64_t r = _x + _y;

    _y = _x ^ _y;
    x = snmalloc::bits::rotl(_x, 24) ^ _y ^ (_y << 16);
    y = snmalloc::bits::rotl(_y, 37);

    return r;
  }

};

using Rand = XorOshiro128Plus;