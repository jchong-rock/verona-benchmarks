#include <memory>
#include <test/harness.h>
#include <cpp/when.h>
#include "util/bench.h"
#include <random>

namespace BOCBenchmark {

namespace {

using verona::cpp::make_cown;
using verona::cpp::cown_ptr;
using verona::cpp::acquired_cown;

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

struct Customer { // things that become cowns do not know their own address
  cown_ptr<CustomerFactory> factory;

  Customer(cown_ptr<CustomerFactory> factory): factory(factory) {}

  static void full(cown_ptr<Customer>);
  void pay_and_leave(cown_ptr<Customer>);
  void wait();
  void sit_down() {};
};

struct Barber {
  uint64_t haircut_rate;
  bool sleeping;

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
  std::deque<cown_ptr<Customer>> customers;
  uint64_t count;
  // rather than storing the customers, just keep count of how many are waiting and use the barbers message queue as the seats
  bool barber_sleeps;
  cown_ptr<Barber> barber;

  WaitingRoom(uint64_t size, cown_ptr<Barber> barber): size(size), count(0), barber_sleeps(true), barber(barber) {}

  static void enter(cown_ptr<WaitingRoom> self, cown_ptr<Customer> customer) {
    when(self) << [customer, tag=self](acquired_cown<WaitingRoom> self) {
      if (self->count == self->size) {
        Customer::full(customer);
      } else {
        self->count++;

        when(self->barber, customer) << [wr_tag=tag, barber_tag=self->barber, customer_tag=customer](acquired_cown<Barber> barber, acquired_cown<Customer> customer) {
          when(wr_tag) << [](acquired_cown<WaitingRoom> wr) { wr->count--; };

          barber->sleeping = false;
          customer->sit_down();
          BusyWaiter(Rand(12345).integer(barber->haircut_rate) + 10);
          customer->pay_and_leave(customer_tag);

          when(barber_tag, wr_tag) << [](acquired_cown<Barber> barber, acquired_cown<WaitingRoom> wr) {
            barber->sleeping = wr->count == 0;
          };
        };
      }
    };
  }
};

void Barber::wait(cown_ptr<Barber> self) { when(self) << [](acquired_cown<Barber>){}; }

void CustomerFactory::returned(cown_ptr<CustomerFactory> self, cown_ptr<Customer> customer) {
  when(self) << [customer](acquired_cown<CustomerFactory> self) {
    self->attempts++;
    WaitingRoom::enter(self->room, customer);
  };
}

// TODO: in verona we don't need to send a message back to the framework
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
      BusyWaiter(Rand(12345).integer(rate) + 10);
    }
  };
}

void Customer::full(cown_ptr<Customer> self) { when(self) << [tag=self](acquired_cown<Customer> self){ CustomerFactory::returned(self->factory, tag); }; }

void Customer::wait() { }

void Customer::pay_and_leave(cown_ptr<Customer> self) { CustomerFactory::left(factory, self); }

};

};