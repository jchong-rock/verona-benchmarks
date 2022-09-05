// use "collections"
// use "random"
// use "../../util"

#include <memory>
#include <test/harness.h>
#include <cpp/when.h>
#include "util/bench.h"
#include <random>

// I'm not really sure whether this is a faithful change
// The benchmark to have queues of pending producers rather than a buffer of data
// and pending producers and then creating a rendezvous between
// a producer and consumer

namespace BOCBenchmark {

namespace {

using namespace std;

#if true
struct Producer;
struct Consumer;

struct Manager {
  uint64_t producer_count;
  uint64_t consumer_count;
  std::deque<cown_ptr<Producer>> availableProducers;
  std::deque<cown_ptr<Consumer>> availableConsumers;
  std::deque<cown_ptr<Producer>> pendingProducers;
  size_t adjusted;

  Manager(uint64_t buffersize, uint64_t producers, uint64_t consumers): producer_count(producers), consumer_count(consumers), adjusted(buffersize - producers) {}

  static cown_ptr<Manager> make_manager(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);
  static void data(cown_ptr<Manager>, cown_ptr<Producer>);
  static void available(cown_ptr<Manager>, cown_ptr<Consumer>);
  static void exit(cown_ptr<Manager>);

  static void rendezvous(cown_ptr<Producer>, cown_ptr<Consumer>);

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

  double produce() {
    last = ItemProcessor(last, costs);
    items--;
    return last;
  }

  void release(cown_ptr<Producer> self) {
    if (items > 0) {
      Manager::data(manager, self);
    } else {
      Manager::exit(manager);
    }
  }
};

struct Consumer {
  double last;
  const cown_ptr<Manager> manager;
  const uint64_t costs;

  Consumer(cown_ptr<Manager> manager, uint64_t costs): last(0), manager(manager), costs(costs) {}

  void consume(double item) {
    last = ItemProcessor(last + item, costs);
  }

  void release(cown_ptr<Consumer> self) {
    Manager::available(manager, self);
  }

};

cown_ptr<Manager> Manager::make_manager(uint64_t buffersize, uint64_t producers, uint64_t consumers, uint64_t items, uint64_t producercosts, uint64_t consumercosts) {
  cown_ptr<Manager> self = make_cown<Manager>(buffersize, producers, consumers);

  when(self) << [tag=self, producers, consumers, items, producercosts, consumercosts](acquired_cown<Manager> self) {
    for (uint64_t i = 0; i < producers; i++)
    {
      cown_ptr<Producer> producer = make_cown<Producer>(tag, items, producercosts);
      Manager::data(tag, producer);
    }

    for (uint64_t i = 0; i < consumers; i++)
    {
      cown_ptr<Consumer> consumer = make_cown<Consumer>(tag, consumercosts);
      self->availableConsumers.push_back(consumer);
    }
  };

  return self;
}

void Manager::complete() {
  if (producer_count == 0 && availableConsumers.size() == consumer_count)
    return;
}

void Manager::rendezvous(cown_ptr<Producer> producer, cown_ptr<Consumer> consumer) {
  when(consumer, producer) << [ctag=consumer, ptag=producer](acquired_cown<Consumer> consumer, acquired_cown<Producer> producer) {
    consumer->consume(producer->produce());
    consumer->release(ctag);
    producer->release(ptag);
  };
}

void Manager::data(cown_ptr<Manager> self, cown_ptr<Producer> producer) {
  when(self) << [producer](acquired_cown<Manager> self) {
    if (self->availableConsumers.empty()) {
      if (self->availableProducers.size() < self->adjusted)
        self->availableProducers.push_back(producer);
      else
        self->pendingProducers.push_back(producer);
    } else {
      cown_ptr<Consumer> consumer = self->availableConsumers.front();
      self->availableConsumers.pop_front();
      Manager::rendezvous(producer, consumer);
    }
  };
}

void Manager::available(cown_ptr<Manager> self, cown_ptr<Consumer> consumer) {
  when(self) << [consumer](acquired_cown<Manager> self) {
    if (self->availableProducers.empty()) {
      self->availableConsumers.push_back(consumer);
      self->complete();
    } else {
      cown_ptr<Producer> producer = self->availableProducers.front();
      self->availableProducers.pop_front();
      Manager::rendezvous(producer, consumer);

      if (!self->pendingProducers.empty()) {
        cown_ptr<Producer> producer = self->pendingProducers.front();
        self->pendingProducers.pop_front();
        when(producer) << [tag=producer](acquired_cown<Producer> producer){ producer->release(tag); };
        // self->availableProducers.push_back(producer);
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
#else

// This is also not helpful, if we're not doing a rendezvous then we can do it with actors

struct Producer;
struct Consumer;

struct Manager {
  uint64_t producer_count;
  uint64_t consumer_count;
  std::deque<double> pendingData;
  std::deque<unique_ptr<Producer>> pendingProducers;
  std::deque<unique_ptr<Consumer>> pendingConsumers;
  size_t adjusted;

  Manager(uint64_t buffersize, uint64_t producers, uint64_t consumers): producer_count(producers), consumer_count(consumers), adjusted(buffersize - producers) {}

  static cown_ptr<Manager> make_manager(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);
  static void data(cown_ptr<Manager>, unique_ptr<Producer>);
  static void available(cown_ptr<Manager>, unique_ptr<Consumer>);
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

  static double produce(unique_ptr<Producer> producer) {
    producer->last = ItemProcessor(producer->last, producer->costs);
    producer->items--;
    if (producer->items > 0) {
      Manager::data(producer->manager, move(producer));
    } else {
      Manager::exit(producer->manager);
    }
    return producer->last;
  }
};

struct Consumer {
  double last;
  const cown_ptr<Manager> manager;
  const uint64_t costs;

  Consumer(cown_ptr<Manager> manager, uint64_t costs): last(0), manager(manager), costs(costs) {}

  static void consume(unique_ptr<Consumer> consumer, double item) {
    consumer->last = ItemProcessor(consumer->last + item, consumer->costs);
    Manager::available(consumer->manager, move(consumer));
  }
};

cown_ptr<Manager> Manager::make_manager(uint64_t buffersize, uint64_t producers, uint64_t consumers, uint64_t items, uint64_t producercosts, uint64_t consumercosts) {
  cown_ptr<Manager> self = make_cown<Manager>(buffersize, producers, consumers);

  when(self) << [tag=self, producers, consumers, items, producercosts, consumercosts](acquired_cown<Manager> self) {
    for (uint64_t i = 0; i < producers; i++)
    {
      Manager::data(tag, make_unique<Producer>(tag, items, producercosts));
    }

    for (uint64_t i = 0; i < consumers; i++)
    {
      self->pendingConsumers.push_back(make_unique<Consumer>(tag, consumercosts));
    }
  };

  return self;
}

void Manager::complete() {
  if (producer_count == 0 && pendingConsumers.size() == consumer_count) {
    std::cout << "fin" << std::endl;
    return;
  }
}

void Manager::data(cown_ptr<Manager> self, unique_ptr<Producer> producer) {
  when(self) << [producer=move(producer)](acquired_cown<Manager> self) mutable {
    if (self->pendingConsumers.empty()) {
      if (self->pendingData.size() >= self->adjusted)
        self->pendingProducers.push_back(move(producer));
      else
        self->pendingData.push_back(Producer::produce(move(producer)));
    } else {
      unique_ptr<Consumer> consumer = move(self->pendingConsumers.front());
      self->pendingConsumers.pop_front();
      Consumer::consume(move(consumer), Producer::produce(move(producer)));
    }
  };
}

void Manager::available(cown_ptr<Manager> self, unique_ptr<Consumer> consumer) {
  when(self) << [consumer=move(consumer)](acquired_cown<Manager> self) mutable {
    if (self->pendingData.empty()) {
      self->pendingConsumers.push_back(move(consumer));
      self->complete();
    } else {
      double item = move(self->pendingData.front());
      self->pendingData.pop_front();
      Consumer::consume(move(consumer), item);
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

#endif
};

};