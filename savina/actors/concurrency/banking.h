#include "util/bench.h"
#include "util/random.h"

namespace actor_benchmark {

namespace banking {

struct Account;
struct Teller;

struct StashToken {
  virtual void requeue(cown_ptr<Account>&)=0;
  virtual ~StashToken() {}
};

struct Account {
  uint64_t index;
  double balance;
  std::deque<std::unique_ptr<StashToken>> stash;
  bool stash_mode;

  Account(uint64_t index, double balance): balance(balance), index(index), stash_mode(false) {}

  static void debit(cown_ptr<Account>&, cown_ptr<Account>&, cown_ptr<Teller>&, double);
  static void credit(cown_ptr<Account>&, cown_ptr<Teller>&, double, cown_ptr<Account>&);
  static void reply(cown_ptr<Account>&, cown_ptr<Teller>&);
};

struct DebitMessage: public StashToken {
  cown_ptr<Account> account;
  cown_ptr<Teller> teller;
  double amount;

  DebitMessage(cown_ptr<Account>& account, cown_ptr<Teller>& teller, double amount): account(account), teller(teller), amount(amount) {}

  void requeue(cown_ptr<Account>& receiver) override { Account::debit(receiver, account, teller, amount); }
};

struct CreditMessage: public StashToken {
  cown_ptr<Account> account;
  cown_ptr<Teller> teller;
  double amount;

  CreditMessage(cown_ptr<Account>& account, cown_ptr<Teller>& teller, double amount): account(account), teller(teller), amount(amount) {}

  void requeue(cown_ptr<Account>& receiver) override { Account::credit(receiver, teller, amount, account); }
};

struct Teller {
  double initial_balance;
  uint64_t transactions;
  SimpleRand random;
  uint64_t completed;
  std::vector<cown_ptr<Account>> accounts;

  Teller(double initial_balance, uint64_t num_accounts, uint64_t transactions, uint64_t seed):
    initial_balance(initial_balance), transactions(transactions), random(SimpleRand(seed)), completed(0) {

    for (uint64_t i = 0; i < num_accounts; i++)
    {
      accounts.push_back(make_cown<Account>(i, initial_balance));
    }
  }

  static void spawn_transactions(cown_ptr<Teller>&& self) {
    when(self) << [tag=self](acquired_cown<Teller> self) mutable {
      for (uint64_t i = 0; i < self->transactions; i++)
      {
        // Must have more than ten accounts for the following maths to work.
        assert(self->accounts.size() > 10);
        // Randomly pick source and destination account
        uint64_t source = self->random.nextInt((self->accounts.size() / 10) * 8);
        uint64_t dest = self->random.nextInt(self->accounts.size() - source);

        if (dest == 0)
          dest++;

        Account::credit(self->accounts[source], tag, self->random.nextDouble() * 1000, self->accounts[source + dest]);
      }
    };
  }

  static void reply(cown_ptr<Teller>& self) {
    when(self) << [](acquired_cown<Teller> self) {
      self->completed++;
      if (self->completed == self->transactions) {
        return;
      }
    };
  }
};

void Account::debit(cown_ptr<Account>& self, cown_ptr<Account>& account, cown_ptr<Teller>& teller, double amount) {
  when(self) << [account, teller, amount](acquired_cown<Account> self) mutable {
    if (!self->stash_mode) {
      self->balance += amount;
      Account::reply(account, teller);
    } else {
      self->stash.push_back(std::make_unique<DebitMessage>(account, teller, amount));
    }
  };
}

void Account::credit(cown_ptr<Account>& self, cown_ptr<Teller>& teller, double amount, cown_ptr<Account>& destination) {
  when(self) << [tag = self, teller, amount, destination](acquired_cown<Account> self) mutable {
    if (!self->stash_mode) {
      self->balance -= amount;
      Account::debit(destination, tag, teller, amount);
      self->stash_mode = true;
    } else {
      self->stash.push_back(std::make_unique<CreditMessage>(destination, teller, amount));
    }
  };
}

void Account::reply(cown_ptr<Account>& self, cown_ptr<Teller>& teller) {
  when(self) << [tag=self, teller](acquired_cown<Account> self) mutable {
    Teller::reply(teller);
    while(!self->stash.empty())
    {
      std::unique_ptr<StashToken> token = std::move(self->stash.front());
      self->stash.pop_front();
      token->requeue(tag);
    }
    self->stash_mode = false;
  };
}

};

struct Banking: public AsyncBenchmark {
  uint64_t accounts;
  uint64_t transactions;
  double initial;

  Banking(uint64_t accounts, uint64_t transactions): accounts(accounts), transactions(transactions) {
    initial = DBL_MAX / float(accounts * transactions);
  }

  void run() {
    auto seed = BenchmarkHarness::get_seed();
    banking::Teller::spawn_transactions(make_cown<banking::Teller>(initial, accounts, transactions, seed));
  }

  std::string name() { return "Banking"; }
};

};