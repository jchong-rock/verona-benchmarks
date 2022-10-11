#include "util/bench.h"
#include "util/random.h"

namespace boc_benchmark {

namespace banking {

/*
 * The original bank required stashing and agreement for 2PC between banks
 * With BoC we can drop most of the messages for the accounts, credit, debit, reply
 */

struct Account;
struct Teller;

struct Account {
  double balance;

  Account(double balance): balance(balance) {}

  void debit(double amount) { balance -= amount; }

  void credit(double amount) { balance += amount; }
};

struct Teller {
  double initial_balance;
  uint64_t transactions;
  SimpleRand random;
  uint64_t completed;
  std::vector<cown_ptr<Account>> accounts;

  Teller(double initial_balance, uint64_t num_accounts, uint64_t transactions):
    initial_balance(initial_balance), transactions(transactions), random(SimpleRand(123456)), completed(0) {

    for (uint64_t i = 0; i < num_accounts; i++)
    {
      accounts.emplace_back(make_cown<Account>(initial_balance));
    }
  }

  static void spawn_transactions(const cown_ptr<Teller>& self) {
    when(self) << [tag=self](acquired_cown<Teller> self)  mutable {
      for (uint64_t i = 0; i < self->transactions; i++)
      {
        // Randomly pick source and destination account
        uint64_t source;
        uint64_t dest;

        do { // changed from actors to avoid deadlock from aliasing
          source = self->random.nextInt((self->accounts.size() / 10) * 8);
          dest = self->random.nextInt(self->accounts.size() - source);
        } while(source == dest);

        if (dest == 0)
          dest++;

        const cown_ptr<Account>& src = self->accounts[source];
        const cown_ptr<Account>& dst = self->accounts[dest];
        double amount = self->random.nextDouble() * 1000;

        when(src, dst) << [amount, tag](acquired_cown<Account> src, acquired_cown<Account> dst) mutable {
          src->debit(amount);
          dst->credit(amount);
          Teller::reply(tag);
        };

      }
    };
  }

  static void reply(const cown_ptr<Teller>& self) {
    when(self) << [](acquired_cown<Teller> self)  mutable {
      self->completed++;
      if (self->completed == self->transactions) {
        return;
      }
    };
  }
};

};

struct Banking: public AsyncBenchmark {
  uint64_t accounts;
  uint64_t transactions;
  double initial;

  Banking(uint64_t accounts, uint64_t transactions): accounts(accounts), transactions(transactions) {
    initial = DBL_MAX / float(accounts * transactions);
  }

  void run() {
    using namespace banking;
    Teller::spawn_transactions(make_cown<Teller>(initial, accounts, transactions));
  }

  std::string name() { return "Banking"; }
};


};