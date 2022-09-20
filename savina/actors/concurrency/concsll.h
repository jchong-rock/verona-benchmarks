#include "util/bench.h"
#include "util/random.h"
#include <list>

namespace ActorBenchmark {

namespace {

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

struct ListMaster;
struct SortedList;

struct ListWorker {
  cown_ptr<ListMaster> listmaster;
  uint64_t size;
  uint64_t write;
  cown_ptr<SortedList> list;
  SimpleRand random;
  uint64_t messages;

  ListWorker(cown_ptr<ListMaster> listmaster, uint64_t messages, uint64_t size, uint64_t write, cown_ptr<SortedList> list):
    listmaster(listmaster), size(size), write(write), list(list), random(messages + size + write), messages(messages) {}

  static void work(cown_ptr<ListWorker>, uint64_t value = 0);
};

struct SortedList {
  SortedLinkedList<uint64_t> data;

  static void write(cown_ptr<SortedList> self, cown_ptr<ListWorker> listworker, uint64_t value) {
    when(self) << [listworker, value](acquired_cown<SortedList> self) {
      self->data.push(value);
      ListWorker::work(listworker, value);
    };
  }

  static void contains(cown_ptr<SortedList> self, cown_ptr<ListWorker> listworker, uint64_t value) {
    when(self) << [listworker, value](acquired_cown<SortedList> self) {
      ListWorker::work(listworker, self->data.contains(value) ? 0 : 1);
    };
  }

  static void size(cown_ptr<SortedList> self, cown_ptr<ListWorker> listworker) {
    when(self) << [listworker](acquired_cown<SortedList> self) {
      ListWorker::work(listworker, self->data.size());
    };
  }
};

struct ListMaster {
  uint64_t listworkers;
  cown_ptr<SortedList> list;

  ListMaster(uint64_t listworkers, cown_ptr<SortedList> list): listworkers(listworkers), list(list) {}

  static void make(uint64_t listworkers, uint64_t messages, uint64_t size, uint64_t write) {
    cown_ptr<SortedList> list = make_cown<SortedList>();

    cown_ptr<ListMaster> listmaster = make_cown<ListMaster>(listworkers, list);

    for (uint64_t i = 0; i < listworkers; ++i) {
      ListWorker::work(make_cown<ListWorker>(listmaster, messages, size, write, list));
    }
  }

  static void done(cown_ptr<ListMaster> self) {
    when(self) << [](acquired_cown<ListMaster> self) {
      if (--self->listworkers == 0) {
        /* done */
      }
    };
  }
};

void ListWorker::work(cown_ptr<ListWorker> self, uint64_t value) {
  when(self) << [tag=self, value](acquired_cown<ListWorker> self) {
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
      ListMaster::done(self->listmaster);
    }
  };
}

struct Concsll: public AsyncBenchmark {
  uint64_t listworkers;
  uint64_t messages;
  uint64_t size;
  uint64_t write;

  Concsll(uint64_t listworkers, uint64_t messages, uint64_t size, uint64_t write):
    listworkers(listworkers), messages(messages), size(size), write(write) {}

  void run() {
    ListMaster::make(listworkers, messages, size, write);
  }

  std::string name() { return "Concurrent Sorted Linked-List"; }
};

};

};
