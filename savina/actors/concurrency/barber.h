#include "util/bench.h"
#include <random>
#include <chrono>

namespace actor_benchmark {

namespace barber {

using namespace verona::cpp;
using namespace std;
using namespace std::chrono;

struct Customer;
struct WaitingRoom;

struct CustomerFactory {
  uint64_t number_of_haircuts;
  uint64_t attempts;
  cown_ptr<WaitingRoom> room;
  Rand random;

  CustomerFactory(uint64_t number_of_haircuts, cown_ptr<WaitingRoom>&& room):
    number_of_haircuts(number_of_haircuts), attempts(0), room(room) {}

  static void run(cown_ptr<CustomerFactory>&&, uint64_t);
  static void returned(cown_ptr<CustomerFactory>&, cown_ptr<Customer>);
  static void left(cown_ptr<CustomerFactory>&);
};

struct Customer {
  cown_ptr<CustomerFactory> factory;

  Customer(cown_ptr<CustomerFactory>& factory): factory(factory) {}

  static void full(cown_ptr<Customer>&);
  static void wait(cown_ptr<Customer>&);
  static void sit_down(cown_ptr<Customer>&);
  static void pay_and_leave(cown_ptr<Customer>&);
};

struct Barber {
  uint64_t haircut_rate;
  Rand random;

  Barber(uint64_t haircut_rate): haircut_rate(haircut_rate) {}

  static void enter(cown_ptr<Barber>&, cown_ptr<Customer>, cown_ptr<WaitingRoom>);
  static void wait(cown_ptr<Barber>&);
};

struct WaitingRoom {
  uint64_t size;
  std::deque<cown_ptr<Customer>> customers;
  bool barber_sleeps;
  cown_ptr<Barber> barber;

  WaitingRoom(uint64_t size, cown_ptr<Barber>&& barber): size(size), barber_sleeps(true), barber(barber) {}

  static void enter(cown_ptr<WaitingRoom>& self, cown_ptr<Customer> customer) {
    when(self) << [tag=self, customer=move(customer)](acquired_cown<WaitingRoom> self) mutable {
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

  static void next(cown_ptr<WaitingRoom>& self) {
    when(self) << [tag=self](acquired_cown<WaitingRoom> self)  mutable {
      if (self->customers.size() > 0) {
        Barber::enter(self->barber, move(self->customers.front()), move(tag));
        self->customers.pop_front();
      } else {
        Barber::wait(self->barber);
        self->barber_sleeps = true;
      }
    };
  }
};

SNMALLOC_SLOW_PATH
static uint64_t BusyWaiter(uint64_t wait, Rand& random) {
  uint64_t x = 0;

  for (uint64_t i = 0; i < wait; ++i) {
    random.next();
    x++;
  }

  return x;
}

void Barber::enter(cown_ptr<Barber>& self, cown_ptr<Customer> customer, cown_ptr<WaitingRoom> room) {
  when(self) << [customer=move(customer), room=move(room)](acquired_cown<Barber> self)  mutable {
    Customer::sit_down(customer);
    BusyWaiter(self->random.integer(self->haircut_rate) + 10, self->random);
    Customer::pay_and_leave(customer);
    WaitingRoom::next(room);
  };
}

void Barber::wait(cown_ptr<Barber>& self) { when(self) << [](acquired_cown<Barber>) mutable {}; }

void CustomerFactory::returned(cown_ptr<CustomerFactory>& self, cown_ptr<Customer> customer) {
  when(self) << [customer=move(customer)](acquired_cown<CustomerFactory> self)  mutable {
    self->attempts++;
    WaitingRoom::enter(self->room, move(customer));
  };
}

void CustomerFactory::left(cown_ptr<CustomerFactory>& self) {
  when(self) << [](acquired_cown<CustomerFactory> self)  mutable {
    self->number_of_haircuts--;
    if (self->number_of_haircuts == 0) {
      return;
    }
  };
}

void CustomerFactory::run(cown_ptr<CustomerFactory>&& self, uint64_t rate) {
  when(self) << [tag=self, rate](acquired_cown<CustomerFactory> self)  mutable {
    for (uint64_t i = 0; i < self->number_of_haircuts; ++i) {
      self->attempts++;
      WaitingRoom::enter(self->room, make_cown<Customer>(tag));
      BusyWaiter(self->random.integer(rate) + 10, self->random);
    }
  };
}

void Customer::full(cown_ptr<Customer>& self) { when(self) << [tag=self](acquired_cown<Customer> self) mutable { CustomerFactory::returned(self->factory, tag); }; }

void Customer::wait(cown_ptr<Customer>& self) { when(self) << [](acquired_cown<Customer>) mutable {}; }

void Customer::sit_down(cown_ptr<Customer>& self) { when(self) << [](acquired_cown<Customer>) mutable {}; }

void Customer::pay_and_leave(cown_ptr<Customer>& self) { when(self) << [](acquired_cown<Customer> self) mutable { CustomerFactory::left(self->factory); }; }

};

struct SleepingBarber: public ActorBenchmark {
  uint64_t haircuts;
  uint64_t room;
  uint64_t production;
  uint64_t cut;

  SleepingBarber(uint64_t haircuts, uint64_t room, uint64_t production, uint64_t cut): haircuts(haircuts), room(room), production(production), cut(cut) {}

  void run() {
    barber::CustomerFactory::run(make_cown<barber::CustomerFactory>(haircuts, make_cown<barber::WaitingRoom>(room, make_cown<barber::Barber>(cut))), production);
  }

  std::string name() { return "Sleeping Barber"; }
};

};