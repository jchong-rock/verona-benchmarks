#include <memory>
#include <test/harness.h>
#include <cpp/when.h>
#include "util/bench.h"
#include <random>

namespace boc_benchmark {

namespace barber {

using namespace verona::cpp;
using namespace std;

#if true

struct Customer;
struct WaitingRoom;

struct CustomerFactory {
  uint64_t number_of_haircuts;
  uint64_t attempts;
  cown_ptr<WaitingRoom> room;

  CustomerFactory(uint64_t number_of_haircuts, const cown_ptr<WaitingRoom>& room):
    number_of_haircuts(number_of_haircuts), attempts(0), room(room) {}

  static void run(const cown_ptr<CustomerFactory>&, uint64_t);
  static void returned(const cown_ptr<CustomerFactory>&, unique_ptr<Customer>);
  static void left(const cown_ptr<CustomerFactory>&, unique_ptr<Customer>);
};

struct Customer {
  cown_ptr<CustomerFactory> factory;

  Customer(const cown_ptr<CustomerFactory>& factory): factory(factory) {}

  static void full(unique_ptr<Customer>);
  static void pay_and_leave(unique_ptr<Customer>);
  // void wait();
  // void sit_down() {};
};

struct Barber {
  uint64_t haircut_rate;

  Barber(uint64_t haircut_rate): haircut_rate(haircut_rate) {}

  static void wait(const cown_ptr<Barber>&);
};

static uint64_t BusyWaiter(uint64_t wait) {
  uint64_t x = 0;

  for (uint64_t i = 0; i < wait; ++i) {
    Rand().next();
    x++;
  }

  return x;
}

struct WaitingRoom {
  const uint64_t size;
  std::deque<unique_ptr<Customer>> customers;
  cown_ptr<Barber> barber;

  WaitingRoom(uint64_t size, const cown_ptr<Barber>& barber): size(size), barber(barber) {}

  static void enter(const cown_ptr<WaitingRoom>& wr, unique_ptr<Customer> customer) {
    when(wr) << [tag=wr, customer=move(customer)](acquired_cown<WaitingRoom> wr) mutable {
      if (wr->customers.size() == wr->size) {
        Customer::full(move(customer));
      } else {
        wr->customers.push_back(move(customer));
        WaitingRoom::next(tag, wr->barber);
      }
    };
  }

  static void next(const cown_ptr<WaitingRoom>& wr, const cown_ptr<Barber>& barber) {
    // starves a customer waiting to sit down
    when(wr, barber) << [wr_tag=wr, barber_tag=barber](acquired_cown<WaitingRoom> wr, acquired_cown<Barber> barber) mutable {
      if (wr->customers.size() > 0) {
        unique_ptr<Customer> customer = move(wr->customers.front());
        wr->customers.pop_front();

        BusyWaiter(Rand(time_point_cast<nanoseconds>(system_clock::now()).time_since_epoch().count()).integer(barber->haircut_rate) + 10);
        Customer::pay_and_leave(move(customer));
        WaitingRoom::next(wr_tag, barber_tag);
      }
    };
  }
};

void Barber::wait(const cown_ptr<Barber>& self) { when(self) << [](acquired_cown<Barber>){}; }

void CustomerFactory::returned(const cown_ptr<CustomerFactory>& factory, unique_ptr<Customer> customer) {
  when(factory) << [customer=move(customer)](acquired_cown<CustomerFactory> factory) mutable {
    factory->attempts++;
    WaitingRoom::enter(factory->room, move(customer));
  };
}

// TODO: in verona we don't need to send a message back to the framework
void CustomerFactory::left(const cown_ptr<CustomerFactory>& factory, unique_ptr<Customer> customer) {
  when(factory) << [](acquired_cown<CustomerFactory> factory) {
    factory->number_of_haircuts--;
    if (factory->number_of_haircuts == 0) {
      // std::cout << "attempts: " << factory->attempts << std::endl;
      return;
    }
  };
}

void CustomerFactory::run(const cown_ptr<CustomerFactory>& factory, uint64_t rate) {
  when(factory) << [tag=factory, rate](acquired_cown<CustomerFactory> factory) mutable {
    for (uint64_t i = 0; i < factory->number_of_haircuts; ++i) {
      factory->attempts++;
      WaitingRoom::enter(factory->room, make_unique<Customer>(tag));
      BusyWaiter(Rand(time_point_cast<nanoseconds>(system_clock::now()).time_since_epoch().count()).integer(rate) + 10);
    }
  };
}

void Customer::full(unique_ptr<Customer> customer) { auto factory = customer->factory; CustomerFactory::returned(factory, move(customer)); }

void Customer::pay_and_leave(unique_ptr<Customer> customer) { auto factory = customer->factory; CustomerFactory::left(factory, move(customer)); }

#else

struct Customer;
struct WaitingRoom;

struct CustomerFactory {
  uint64_t number_of_haircuts;
  uint64_t attempts;
  cown_ptr<WaitingRoom> room;

  CustomerFactory(uint64_t number_of_haircuts, cown_ptr<WaitingRoom> room):
    number_of_haircuts(number_of_haircuts), attempts(0), room(room) {}

  static void run(const cown_ptr<CustomerFactory>&, uint64_t);
  static void returned(const cown_ptr<CustomerFactory>&, const cown_ptr<Customer>&);
  static void left(const cown_ptr<CustomerFactory>&, const cown_ptr<Customer>&);
};

struct Customer { // things that become cowns do not know their own address
  cown_ptr<CustomerFactory> factory;

  Customer(const cown_ptr<CustomerFactory>& factory): factory(factory) {}

  static void full(const cown_ptr<Customer>&);
  void pay_and_leave(const cown_ptr<Customer>&);
  void wait();
  void sit_down() {};
};

struct Barber {
  uint64_t haircut_rate;
  bool busy;

  Barber(uint64_t haircut_rate): haircut_rate(haircut_rate) {}

  static void enter(const cown_ptr<Barber>&, const cown_ptr<Customer>&, const cown_ptr<WaitingRoom>&);
  static void wait(const cown_ptr<Barber>&);
};

struct SleepingBarber: public AsyncBenchmark {
  uint64_t haircuts;
  uint64_t room;
  uint64_t production;
  uint64_t cut;

  SleepingBarber(uint64_t haircuts, uint64_t room, uint64_t production, uint64_t cut): haircuts(haircuts), room(room), production(production), cut(cut) {}

  void run() {
    CustomerFactory::run(make_cown<CustomerFactory>(haircuts, make_cown<WaitingRoom>(room, make_cown<Barber>(cut))), production);
  }

  std::string name() { return "Sleeping Barber"; }
};

static uint64_t BusyWaiter(uint64_t wait) {
  uint64_t x = 0;

  for (uint64_t i = 0; i < wait; ++i) {
    Rand().next();
    x++;
  }

  return x;
}

// #define countversion

struct WaitingRoom {
  const uint64_t size;
#ifndef countversion
  std::deque<cown_ptr<Customer>> customers;
#else
  uint64_t count = 0;
#endif
  cown_ptr<Barber> barber;

  WaitingRoom(uint64_t size, const cown_ptr<Barber>& barber): size(size), barber(barber) {}

#ifndef countversion
  static void enter(const cown_ptr<WaitingRoom>& wr, const cown_ptr<Customer>& customer) {
    when(wr) << [customer, tag=wr](acquired_cown<WaitingRoom> wr) {
      if (wr->customers.size() == wr->size) {
        Customer::full(customer);
      } else {
        wr->customers.push_back(customer);

        WaitingRoom::next(tag, wr->barber);
      }
    };
  }

  static void next(const cown_ptr<WaitingRoom>& wr, const cown_ptr<Barber>& barber) {
    when(wr, barber) << [wr_tag=wr, barber_tag=barber](acquired_cown<WaitingRoom> wr, acquired_cown<Barber> barber) {
      if(!barber->busy) {
        if (wr->customers.size() > 0) {
          cown_ptr<Customer> customer = wr->customers.front();
          wr->customers.pop_front();
          // barber->sleeping = false;
          barber->busy = true;

          when(barber_tag, customer) << [wr_tag, barber_tag, customer_tag=customer](acquired_cown<Barber> barber, acquired_cown<Customer> customer) {
            customer->sit_down();
            BusyWaiter(Rand(time_point_cast<nanoseconds>(system_clock::now()).time_since_epoch().count()).integer(barber->haircut_rate) + 10);
            customer->pay_and_leave(customer_tag);
            barber->busy = false;
            WaitingRoom::next(wr_tag, barber_tag);
          };
        } else if (wr->customers.size() == 0) {
          // barber->sleeping = true;
        }
      }
    };
  }
#else
  static void enter(const cown_ptr<WaitingRoom>& wr, const cown_ptr<Customer>& customer) {
    when(wr) << [customer, tag=wr](acquired_cown<WaitingRoom> wr) {
      if (wr->count == wr->size) {
        Customer::full(customer);
      } else {
        wr->count++;

        when(wr->barber, customer) << [wr_tag=tag, barber_tag=wr->barber, customer_tag=customer](acquired_cown<Barber> barber, acquired_cown<Customer> customer) {
          when(wr_tag) << [](acquired_cown<WaitingRoom> wr) { wr->count--; };

          // barber->sleeping = false;
          customer->sit_down();
          BusyWaiter(Rand(time_point_cast<nanoseconds>(system_clock::now()).time_since_epoch().count()).integer(barber->haircut_rate) + 10);
          customer->pay_and_leave(customer_tag);

          when(barber_tag, wr_tag) << [](acquired_cown<Barber> barber, acquired_cown<WaitingRoom> wr) {
            // barber->sleeping = wr->count == 0;
          };
        };
      }
    };
  }
#endif
};

void Barber::wait(const cown_ptr<Barber>& self) { when(self) << [](acquired_cown<Barber>){}; }

void CustomerFactory::returned(const cown_ptr<CustomerFactory>& self, const cown_ptr<Customer>& customer) {
  when(self) << [customer](acquired_cown<CustomerFactory> self) {
    self->attempts++;
    WaitingRoom::enter(self->room, customer);
  };
}

// TODO: in verona we don't need to send a message back to the framework
void CustomerFactory::left(const cown_ptr<CustomerFactory>& self, const cown_ptr<Customer>& customer) {
  when(self) << [](acquired_cown<CustomerFactory> self) {
    self->number_of_haircuts--;
    if (self->number_of_haircuts == 0) {
      std::cout << "attempts: " << self->attempts << std::endl;
      return;
    }
  };
}

void CustomerFactory::run(const cown_ptr<CustomerFactory>& self, uint64_t rate) {
  when(self) << [tag=self, rate](acquired_cown<CustomerFactory> self) {
    for (uint64_t i = 0; i < self->number_of_haircuts; ++i) {
      self->attempts++;
      WaitingRoom::enter(self->room, make_cown<Customer>(tag));
      BusyWaiter(Rand(time_point_cast<nanoseconds>(system_clock::now()).time_since_epoch().count()).integer(rate) + 10);
    }
  };
}

void Customer::full(const cown_ptr<Customer>& self) { when(self) << [tag=self](acquired_cown<Customer> self){ CustomerFactory::returned(self->factory, tag); }; }

void Customer::wait() { }

void Customer::pay_and_leave(const cown_ptr<Customer>& self) { CustomerFactory::left(factory, self); }
#endif

};

struct SleepingBarber: public AsyncBenchmark {
  uint64_t haircuts;
  uint64_t room;
  uint64_t production;
  uint64_t cut;

  SleepingBarber(uint64_t haircuts, uint64_t room, uint64_t production, uint64_t cut): haircuts(haircuts), room(room), production(production), cut(cut) {}

  void run() {
    using namespace barber;
    CustomerFactory::run(make_cown<CustomerFactory>(haircuts, make_cown<WaitingRoom>(room, make_cown<Barber>(cut))), production);
  }

  std::string name() { return "Sleeping Barber"; }
};


};