// use "collections"
// use "random"
// use "../../util"

#include <memory>
#include <test/harness.h>
#include <cpp/when.h>
#include "util/bench.h"
#include <random>

namespace ActorBenchmark {

struct Producer;
struct Consumer;

struct Manager {
  uint64_t producer_count;
  std::deque<cown_ptr<Producer>> producers;
  std::deque<cown_ptr<Consumer>> consumers;
  std::deque<cown_ptr<Producer>> availableProducers;
  std::deque<cown_ptr<Consumer>> availableConsumers;
  std::deque<std::tuple<cown_ptr<Producer>, double>> pendingData;
  size_t adjusted;

  Manager(uint64_t buffersize, uint64_t producers): producer_count(producers), adjusted(buffersize - producers) {}

  static cown_ptr<Manager> make_manager(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);
  static void data(cown_ptr<Manager>, cown_ptr<Producer>, double item);
  static void available(cown_ptr<Manager>, cown_ptr<Consumer>);
  static void exit(cown_ptr<Manager>);

  void complete();
};

static double ItemProcessor(double current, uint64_t cost) {
  double result = current;
  Rand random(cost);

  if (cost > 0) {
    for (uint64_t i = 0; i < cost; i++)
    {
      for (uint64_t j = 0; j < 100; j++)
      {
        result += std::log(((double)random.next()) + 0.01);
      }
    }
  } else {
    result += std::log(((double)random.next()) + 0.01);
  }

  return result;
}

struct Producer {
  double last;
  uint64_t items;
  const cown_ptr<Manager> manager;
  const uint64_t costs;

  Producer(cown_ptr<Manager> manager, uint64_t items, uint64_t costs): last(0), items(items), manager(manager), costs(costs){}

  static void produce(cown_ptr<Producer> self) {
    when(self) << [tag=self](acquired_cown<Producer> self) {
      if (self->items > 0) {
        self->last = ItemProcessor(self->last, self->costs);
        Manager::data(self->manager, tag, self->last);
        self->items--;
      } else {
        Manager::exit(self->manager);
      }
    };
  }
};

struct Consumer {
  double last;
  const cown_ptr<Manager> manager;
  const uint64_t costs;

  Consumer(cown_ptr<Manager> manager, uint64_t costs): last(0), manager(manager), costs(costs) {}

  static void data(cown_ptr<Consumer> self, cown_ptr<Producer> producer, double item) {
    when(self) << [tag=self, item](acquired_cown<Consumer> self){
      self->last = ItemProcessor(self->last + item, self->costs);
      Manager::available(self->manager, tag);
    };
  }
};

cown_ptr<Manager> Manager::make_manager(uint64_t buffersize, uint64_t producers, uint64_t consumers, uint64_t items, uint64_t producercosts, uint64_t consumercosts) {
  cown_ptr<Manager> self = make_cown<Manager>(buffersize, producers);

  when(self) << [tag=self, producers, consumers, items, producercosts, consumercosts](acquired_cown<Manager> self) {
    for (uint64_t i = 0; i < producers; i++)
    {
      cown_ptr<Producer> producer = make_cown<Producer>(tag, items, producercosts);
      Producer::produce(producer);
      self->producers.push_back(producer);
    }

    for (uint64_t i = 0; i < consumers; i++)
    {
      cown_ptr<Consumer> consumer = make_cown<Consumer>(tag, consumercosts);
      self->consumers.push_back(consumer);
      self->availableConsumers.push_back(consumer);
    }
  };

  return self;
}

void Manager::complete() {
  if (producer_count == 0 && availableConsumers.size() == consumers.size())
    return;
}

void Manager::data(cown_ptr<Manager> self, cown_ptr<Producer> producer, double item) {
  when(self) << [producer, item](acquired_cown<Manager> self) {
    if (self->availableConsumers.empty()) {
      self->pendingData.push_back(std::make_tuple(producer, item));
    } else {
      cown_ptr<Consumer> consumer = self->availableConsumers.front();
      self->availableConsumers.pop_front();
      Consumer::data(consumer, producer, item);
    }

    if (self->pendingData.size() >= self->adjusted) {
      self->availableProducers.push_back(producer);
    } else {
      Producer::produce(producer);
    }
  };
}

void Manager::available(cown_ptr<Manager> self, cown_ptr<Consumer> consumer) {
  when(self) << [consumer](acquired_cown<Manager> self) {
    if (self->pendingData.empty()) {
      self->availableConsumers.push_back(consumer);
      self->complete();
    } else {
      cown_ptr<Producer> producer;
      double item;
      std::tie(producer, item) = self->pendingData.front();
      self->pendingData.pop_front();
      Consumer::data(consumer, producer, item);

      if (!self->availableProducers.empty()) {
        cown_ptr<Producer> producer = self->availableProducers.front();
        self->availableProducers.pop_front();
        Producer::produce(producer);
      }
    }
  };
}

void Manager::exit(cown_ptr<Manager> self) {
  when(self) << [](acquired_cown<Manager> self) {
    self->producer_count--;
    self->complete();
  };
}

struct BndBuffer: public AsyncBenchmark {
  const uint64_t buffersize;
  const uint64_t producers;
  const uint64_t consumers;
  const uint64_t items;
  const uint64_t producercosts;
  const uint64_t consumercosts;

  BndBuffer(uint64_t buffersize, uint64_t producers, uint64_t consumers, uint64_t items, uint64_t producercosts, uint64_t consumercosts):
    buffersize(buffersize), producers(producers), consumers(consumers), items(items), producercosts(producercosts), consumercosts(consumercosts) {}

  void run() {
    Manager::make_manager(buffersize, producers, consumers, items, producercosts, consumercosts);
  }

  std::string name() { return "Bounded Buffer"; }
};

};