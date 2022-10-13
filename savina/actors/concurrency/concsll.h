#include "util/bench.h"
#include "util/random.h"
#include <list>

namespace actor_benchmark {

namespace concsll {

using namespace std;

template<typename T>
struct SortedLinkedList {
  list<T> data;

  void push(T value) {
    if (data.empty()) {
      data.push_back(value);
    } else {
      auto it = data.begin();
      while (it != data.end()) {
        if (*it <= value) {
          data.insert(it, value);
          return;
        }
        it++;
      }
    }
  }

  bool contains(T value) {
    return std::find(data.begin(), data.end(), value) != data.end();
  }

  uint64_t size() {
    return data.size();
  }
};

struct Master;
struct SortedList;

struct Worker {
  const cown_ptr<Master> master;
  uint64_t size;
  uint64_t write;
  const cown_ptr<SortedList> list;
  SimpleRand random;
  uint64_t messages;

  Worker(const cown_ptr<Master> master, uint64_t messages, uint64_t size, uint64_t write, const cown_ptr<SortedList> list):
    master(move(master)), size(size), write(write), list(move(list)), random(messages + size + write), messages(messages) {}

  static void work(const cown_ptr<Worker>&, uint64_t value = 0);
};

struct SortedList {
  SortedLinkedList<uint64_t> data;

  static void write(const cown_ptr<SortedList>& self, const cown_ptr<Worker> worker, uint64_t value) {
    when(self) << [worker=move(worker), value](acquired_cown<SortedList> self)  mutable {
      self->data.push(value);
      Worker::work(worker, value);
    };
  }

  static void contains(const cown_ptr<SortedList>& self, const cown_ptr<Worker> worker, uint64_t value) {
    when(self) << [worker=move(worker), value](acquired_cown<SortedList> self)  mutable {
      Worker::work(worker, self->data.contains(value) ? 0 : 1);
    };
  }

  static void size(const cown_ptr<SortedList>& self, const cown_ptr<Worker> worker) {
    when(self) << [worker=move(worker)](acquired_cown<SortedList> self)  mutable {
      Worker::work(worker, self->data.size());
    };
  }
};

struct Master {
  uint64_t workers;
  const cown_ptr<SortedList> list;

  Master(uint64_t workers, const cown_ptr<SortedList> list): workers(workers), list(move(list)) {}

  static void make(uint64_t workers, uint64_t messages, uint64_t size, uint64_t write) {
    const cown_ptr<SortedList> list = make_cown<SortedList>();

    const cown_ptr<Master> master = make_cown<Master>(workers, list);

    for (uint64_t i = 0; i < workers - 1; ++i) {
      Worker::work(make_cown<Worker>(master, messages, size, write, list));
    }

    // assume workers is > 0
    Worker::work(make_cown<Worker>(move(master), messages, size, write, move(list)));
  }

  static void done(const cown_ptr<Master>& self) {
    when(self) << [](acquired_cown<Master> self)  mutable {
      if (--self->workers == 0) {
        /* done */
      }
    };
  }
};

void Worker::work(const cown_ptr<Worker>& self, uint64_t value) {
  when(self) << [tag=self, value](acquired_cown<Worker> self)  mutable {
    if (--self->messages > 0) {
      uint64_t value2 = self->random.nextInt(100);

      if (value2 < self->size) {
        SortedList::size(self->list, move(tag));
      } else if (value2 < (self->size + self->write)) {
        SortedList::write(self->list, move(tag), value2);
      } else {
        SortedList::contains(self->list, move(tag), value2);
      }
    } else {
      Master::done(self->master);
    }
  };
}

};


struct Concsll: public AsyncBenchmark {
  uint64_t workers;
  uint64_t messages;
  uint64_t size;
  uint64_t write;

  Concsll(uint64_t workers, uint64_t messages, uint64_t size, uint64_t write):
    workers(workers), messages(messages), size(size), write(write) {}

  void run() {
    concsll::Master::make(workers, messages, size, write);
  }

  std::string name() { return "Concurrent Sorted Linked-List"; }
};


};
