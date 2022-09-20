#include <memory>
#include <test/harness.h>
#include <cpp/when.h>
#include "util/bench.h"
#include <random>
#include <chrono>

namespace ActorBenchmark {

namespace {

using namespace verona::cpp;
using namespace std;
using namespace std::chrono;

struct Customer;
struct WaitingRoom;

struct CustomerFactory {
  uint64_t number_of_haircuts;
  uint64_t attempts;
  cown_ptr<WaitingRoom> room;

  CustomerFactory(uint64_t number_of_haircuts, cown_ptr<WaitingRoom> room):
    number_of_haircuts(number_of_haircuts), attempts(0), room(room) {}

  static void run(cown_ptr<CustomerFactory>, uint64_t);
  static void returned(cown_ptr<CustomerFactory>, cown_ptr<Customer>);
  static void left(cown_ptr<CustomerFactory>, cown_ptr<Customer>);
};

struct Customer {
  cown_ptr<CustomerFactory> factory;

  Customer(cown_ptr<CustomerFactory> factory): factory(factory) {}

  static void full(cown_ptr<Customer>);
  static void wait(cown_ptr<Customer>);
  static void sit_down(cown_ptr<Customer>);
  static void pay_and_leave(cown_ptr<Customer>);
};

struct Barber {
  uint64_t haircut_rate;

  Barber(uint64_t haircut_rate): haircut_rate(haircut_rate) {}

  static void enter(cown_ptr<Barber>, cown_ptr<Customer>, cown_ptr<WaitingRoom>);
  static void wait(cown_ptr<Barber>);
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

struct WaitingRoom {
  uint64_t size;
  std::deque<cown_ptr<Customer>> customers;
  bool barber_sleeps;
  cown_ptr<Barber> barber;

  WaitingRoom(uint64_t size, cown_ptr<Barber> barber): size(size), barber_sleeps(true), barber(barber) {}

  static void enter(cown_ptr<WaitingRoom> self, cown_ptr<Customer> customer) {
    when(self) << [customer, tag=self](acquired_cown<WaitingRoom> self) {
      if (self->customers.size() == self->size) {
        Customer::full(customer);
      } else {
        self->customers.push_back(customer);

        if (self->barber_sleeps) {
          self->barber_sleeps = false;
          WaitingRoom::next(tag);
        } else {
          Customer::wait(customer);
        }
      }
    };
  }

  static void next(cown_ptr<WaitingRoom> self) {
    when(self) << [tag=self](acquired_cown<WaitingRoom> self) {
      if (self->customers.size() > 0) {
        Barber::enter(self->barber, self->customers.front(), tag);
        self->customers.pop_front();
      } else {
        Barber::wait(self->barber);
        self->barber_sleeps = true;
      }
    };
  }
};

static uint64_t BusyWaiter(uint64_t wait) {
  uint64_t x = 0;

  for (uint64_t i = 0; i < wait; ++i) {
    Rand().next();
    x++;
  }

  return x;
}

void Barber::enter(cown_ptr<Barber> self, cown_ptr<Customer> customer, cown_ptr<WaitingRoom> room) {
  when(self) << [customer, room](acquired_cown<Barber> self) {
    Customer::sit_down(customer);
    BusyWaiter(Rand(time_point_cast<nanoseconds>(system_clock::now()).time_since_epoch().count()).integer(self->haircut_rate) + 1000);
    Customer::pay_and_leave(customer);
    WaitingRoom::next(room);
  };
}

void Barber::wait(cown_ptr<Barber> self) { when(self) << [](acquired_cown<Barber>){}; }

void CustomerFactory::returned(cown_ptr<CustomerFactory> self, cown_ptr<Customer> customer) {
  when(self) << [customer](acquired_cown<CustomerFactory> self) {
    self->attempts++;
    WaitingRoom::enter(self->room, customer);
  };
}

void CustomerFactory::left(cown_ptr<CustomerFactory> self, cown_ptr<Customer> customer) {
  when(self) << [](acquired_cown<CustomerFactory> self) {
    self->number_of_haircuts--;
    if (self->number_of_haircuts == 0) {
      return;
    }
  };
}

void CustomerFactory::run(cown_ptr<CustomerFactory> self, uint64_t rate) {
  when(self) << [tag=self, rate](acquired_cown<CustomerFactory> self) {
    for (uint64_t i = 0; i < self->number_of_haircuts; ++i) {
      self->attempts++;
      WaitingRoom::enter(self->room, make_cown<Customer>(tag));
      BusyWaiter(Rand(time_point_cast<nanoseconds>(system_clock::now()).time_since_epoch().count()).integer(rate) + 1000);
    }
  };
}

void Customer::full(cown_ptr<Customer> self) { when(self) << [tag=self](acquired_cown<Customer> self){ CustomerFactory::returned(self->factory, tag); }; }

void Customer::wait(cown_ptr<Customer> self) { when(self) << [](acquired_cown<Customer>){}; }

void Customer::sit_down(cown_ptr<Customer> self) { when(self) << [](acquired_cown<Customer>){}; }

void Customer::pay_and_leave(cown_ptr<Customer> self) { when(self) << [tag=self](acquired_cown<Customer> self){ CustomerFactory::left(self->factory, tag); }; }

};

};