#include <cpp/when.h>
#include <util/bench.h>
#include <util/random.h>

namespace boc_benchmark
{

  struct Block
  {
    uint64_t base;
    std::vector<bool> sieve_block;
    cown_ptr<Block> next;

    bool contains(uint64_t i)
    {
      return (i - base) < sieve_block.size();
    }

    std::vector<bool>::reference get(uint64_t i)
    {
      return sieve_block[i - base];
    }

    void remove(uint64_t i, uint64_t m)
    {
      if (contains(i))
      {
        get(i) = false;
        remove(i + m, m);
      }
      else
      {
        if (next == nullptr)
          return;

        when(next) << [i, m](acquired_cown<Block> next) mutable
        {
          next->remove(i, m);
        };
      }
    }

    void iterate(uint64_t i)
    {
      if (contains(i))
      {
        if (get(i))
        {
          // We have found a prime
          //          std::cout << "Prime: " << i << std::endl;
          remove(i * 2, i);
        }
        iterate(i + 1);
      }
      else
      {
        if (next == nullptr)
          return;

        when(next) << [i](acquired_cown<Block> next) mutable
        {
          next->iterate(i);
        };
      }
    }

    Block(uint64_t base, uint64_t size, cown_ptr<Block> next) : base(base), sieve_block(size, true), next(next) {}
  };

  struct Sieve : public BocBenchmark
  {
    uint64_t size;
    uint64_t buffersize;

    Sieve(uint64_t size, uint64_t buffersize) : size(size), buffersize(buffersize) {}

    void run()
    {
      uint64_t count = (size + buffersize - 1) / buffersize;

      cown_ptr<Block> prev;
      for (size_t i = count - 1;; i--)
      {
        uint64_t base = i * buffersize;
        uint64_t size = std::min(buffersize, size - base);
        prev = make_cown<Block>(base, size, prev);
        if (i == 0)
          break;
      }

      when(prev) << [this](acquired_cown<Block> prev) mutable
      {
        prev->iterate(2);
      };
    }

    std::string name() { return "Sieve of Eratosthenes"; }
  };

}