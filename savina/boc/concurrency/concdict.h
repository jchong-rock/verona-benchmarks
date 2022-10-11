#include "util/bench.h"
#include "util/random.h"

namespace boc_benchmark {

namespace concdict {

using namespace std;

struct Master;

struct Dictionary;

struct Worker {
  cown_ptr<Master> master;
  uint64_t percentage;
  cown_ptr<Dictionary> dictionary;
  SimpleRand random;
  uint64_t messages;

  Worker(const cown_ptr<Master>& master, uint64_t index, const cown_ptr<Dictionary>& dictionary, uint64_t messages, uint64_t percentage):
    master(master), percentage(percentage), dictionary(dictionary), random(index + messages + percentage), messages(messages) {}

  static void work(const cown_ptr<Worker>& self, uint64_t value = 0);
};

struct Dictionary {
  unordered_map<uint64_t, uint64_t> map;
  Dictionary() {}
  static void write(const cown_ptr<Dictionary>& self, const cown_ptr<Worker>& worker, uint64_t key, uint64_t value);
  static void read(const cown_ptr<Dictionary>& self, const cown_ptr<Worker>& worker, uint64_t key);
};

struct Master {
  uint64_t workers;
  Master(uint64_t workers): workers(workers) {}

  static cown_ptr<Master> make(uint64_t workers, uint64_t messages, uint64_t percentage) {
    auto master = make_cown<Master>(workers);
    when(master) << [tag=master, workers, messages, percentage](acquired_cown<Master>)  mutable{

      auto dictionary = make_cown<Dictionary>();

      for (uint64_t i = 0; i < workers; ++i) {
        Worker::work(make_cown<Worker>(tag, i, dictionary, messages, percentage));
      }
    };
    return master;
  }

  static void done(const cown_ptr<Master>& self) {
    when(self) << [](acquired_cown<Master> self)  mutable{
      if (self->workers-- == 1) {
        /* done */
      }
    };
  }
};

void Worker::work(const cown_ptr<Worker>& self, uint64_t value) {
  when(self) << [tag=self, value](acquired_cown<Worker> self)  mutable{
    if (self->messages-- >= 1) {
      uint64_t value = self->random.nextInt(100);
      value %= (INT64_MAX / 4096);

      if (value < self->percentage) {
        Dictionary::write(self->dictionary, tag, value, value);
      } else {
        Dictionary::read(self->dictionary, tag, value);
      }
    } else {
      Master::done(self->master);
    }
  };
}

void Dictionary::write(const cown_ptr<Dictionary>& self, const cown_ptr<Worker>& worker, uint64_t key, uint64_t value) {
  when(self) << [worker, key, value](acquired_cown<Dictionary> self) mutable {
    self->map[key] = value;
    Worker::work(worker, value);
  };
}

// Somehow read-only makes it slower?
void Dictionary::read(const cown_ptr<Dictionary>& self, const cown_ptr<Worker>& worker, uint64_t key) {
  when(verona::cpp::read(self)) << [worker, key](acquired_cown<const Dictionary> self)  mutable{
    auto it = self->map.find(key);
    #if true
    Worker::work(worker, it != self->map.end() ? it->second : 0);
    #else
    // busy work TBD
    #endif
  };
}

};

struct Concdict: AsyncBenchmark {
  uint64_t workers;
  uint64_t messages;
  uint64_t percentage;

  Concdict(uint64_t workers, uint64_t messages, uint64_t percentage):
    workers(workers), messages(messages), percentage(percentage) {};

  void run() {
    concdict::Master::make(workers, messages, percentage);
  }

  std::string name() { return "Concurrent Dictionary"; }
};

};