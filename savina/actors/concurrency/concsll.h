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
  cown_ptr<Master> master;
  uint64_t size;
  uint64_t write;
  cown_ptr<SortedList> list;
  SimpleRand random;
  uint64_t messages;

  Worker(cown_ptr<Master> master, uint64_t messages, uint64_t size, uint64_t write, cown_ptr<SortedList> list):
    master(master), size(size), write(write), list(list), random(messages + size + write), messages(messages) {}

  static void work(cown_ptr<Worker>, uint64_t value = 0);
};

struct SortedList {
  SortedLinkedList<uint64_t> data;

  static void write(cown_ptr<SortedList> self, cown_ptr<Worker> worker, uint64_t value) {
    when(self) << [worker, value](acquired_cown<SortedList> self) {
      self->data.push(value);
      Worker::work(worker, value);
    };
  }

  static void contains(cown_ptr<SortedList> self, cown_ptr<Worker> worker, uint64_t value) {
    when(self) << [worker, value](acquired_cown<SortedList> self) {
      Worker::work(worker, self->data.contains(value) ? 0 : 1);
    };
  }

  static void size(cown_ptr<SortedList> self, cown_ptr<Worker> worker) {
    when(self) << [worker](acquired_cown<SortedList> self) {
      Worker::work(worker, self->data.size());
    };
  }
};

struct Master {
  uint64_t workers;
  cown_ptr<SortedList> list;

  Master(uint64_t workers, cown_ptr<SortedList> list): workers(workers), list(list) {}

  static void make(uint64_t workers, uint64_t messages, uint64_t size, uint64_t write) {
    cown_ptr<SortedList> list = make_cown<SortedList>();

    cown_ptr<Master> master = make_cown<Master>(workers, list);

    for (uint64_t i = 0; i < workers; ++i) {
      Worker::work(make_cown<Worker>(master, messages, size, write, list));
    }
  }

  static void done(cown_ptr<Master> self) {
    when(self) << [](acquired_cown<Master> self) {
      if (--self->workers == 0) {
        /* done */
      }
    };
  }
};

void Worker::work(cown_ptr<Worker> self, uint64_t value) {
  when(self) << [tag=self, value](acquired_cown<Worker> self) {
    if (--self->messages > 0) {
      uint64_t value2 = self->random.nextInt(100);

      if (value2 < self->size) {
        SortedList::size(self->list, tag);
      } else if (value2 < (self->size + self->write)) {
        SortedList::write(self->list, tag, value2);
      } else {
        SortedList::contains(self->list, tag, value2);
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
