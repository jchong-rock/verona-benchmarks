#include <cpp/when.h>
#include <util/bench.h>
#include <util/random.h>

namespace boc_benchmark
{

  class Block
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

    template<typename F>
    void apply(uint64_t i, F f)
    {
      if (contains(i))
        f(this, i);
      else
      {
        if (next)
        {
          when (next) << [i, f=std::move(f)](acquired_cown<Block> next) mutable {
            next->apply(i, std::move(f));
          };
        }
      }
    }

  public:
    void remove(uint64_t i, uint64_t m)
    {
      apply(i, [m](Block* self, uint64_t i) {
        self->get(i) = false;
        self->remove(i + m, m);
      });
    }

    void iterate(uint64_t i)
    {
      apply(i, [](Block* self, uint64_t i) {
        if (self->get(i))
        {
          // We have found a prime
          // std::cout << "Prime: " << i << std::endl;
          self->remove(i * 2, i);
        }
        self->iterate(i + 1);
      });
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