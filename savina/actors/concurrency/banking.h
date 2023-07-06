#include "util/bench.h"
#include "util/random.h"

namespace actor_benchmark {

namespace banking {

struct Account;
struct Teller;

struct StashToken {
  virtual void requeue(cown_ptr<Account>)=0;
  virtual ~StashToken() {}
};

struct Account {
  uint64_t index;
  double balance;
  std::deque<std::unique_ptr<StashToken>> stash;
  bool stash_mode;

  Account(uint64_t index, double balance): balance(balance), index(index), stash_mode(false) {}

  static void debit(cown_ptr<Account>, cown_ptr<Account>, cown_ptr<Teller>, double);
  static void credit(cown_ptr<Account>, cown_ptr<Teller>, double, cown_ptr<Account>);
  static void reply(cown_ptr<Account>, cown_ptr<Teller>);
  void unstash(cown_ptr<Account>);
};

struct DebitMessage: public StashToken {
  cown_ptr<Account> account;
  cown_ptr<Teller> teller;
  double amount;

  DebitMessage(cown_ptr<Account> account, cown_ptr<Teller> teller, double amount): account(std::move(account)), teller(std::move(teller)), amount(amount) {}

  void requeue(cown_ptr<Account> receiver) override { Account::debit(std::move(receiver), std::move(account), std::move(teller), amount); }
};

struct CreditMessage: public StashToken {
  cown_ptr<Account> account;
  cown_ptr<Teller> teller;
  double amount;

  CreditMessage(cown_ptr<Account> account, cown_ptr<Teller> teller, double amount): account(std::move(account)), teller(std::move(teller)), amount(amount) {}

  void requeue(cown_ptr<Account> receiver) override { Account::credit(std::move(receiver), std::move(teller), amount, std::move(account)); }
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
      accounts.emplace_back(make_cown<Account>(i, initial_balance));
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

  static void reply(cown_ptr<Teller> self) {
    when(self) << [](acquired_cown<Teller> self) {
      self->completed++;
      if (self->completed == self->transactions) {
        return;
      }
    };
  }
};

void Account::debit(cown_ptr<Account> self, cown_ptr<Account> account, cown_ptr<Teller> teller, double amount) {
  when(self) << [tag = self, account = std::move(account), teller = std::move(teller), amount](acquired_cown<Account> self) mutable {
    if (!self->stash_mode) {
      self->balance += amount;
      Account::reply(std::move(account), std::move(teller));
      self->unstash(tag);
    } else {
      self->stash.emplace_back(std::make_unique<DebitMessage>(std::move(account), std::move(teller), amount));
    }
  };
}

void Account::credit(cown_ptr<Account> self, cown_ptr<Teller> teller, double amount, cown_ptr<Account> destination) {
  when(self) << [teller = std::move(teller), amount, destination = std::move(destination)](acquired_cown<Account> self) mutable {
    if (!self->stash_mode) {
      self->balance -= amount;
      Account::debit(std::move(destination), self.cown(), std::move(teller), amount);
      self->stash_mode = true;
    } else {
      self->stash.emplace_back(std::make_unique<CreditMessage>(std::move(destination), std::move(teller), amount));
    }
  };
}

void Account::unstash(cown_ptr<Account> tag) {
  if(!stash.empty())
  {
    std::unique_ptr<StashToken> token = std::move(stash.front());
    stash.pop_front();
    token->requeue(tag);
  }
}

void Account::reply(cown_ptr<Account> self, cown_ptr<Teller> teller) {
  when(self) << [tag=self, teller](acquired_cown<Account> self) mutable {
    Teller::reply(teller);
    self->unstash(tag);
    self->stash_mode = false;
  };
}

};

struct Banking: public ActorBenchmark {
  uint64_t accounts;
  uint64_t transactions;
  double initial;
  inline static const std::string name = "Banking";

  Banking(uint64_t accounts, uint64_t transactions): accounts(accounts), transactions(transactions) {
    initial = DBL_MAX / float(accounts * transactions);
  }

  void run() {
    auto seed = BenchmarkHarness::get_seed();
    banking::Teller::spawn_transactions(make_cown<banking::Teller>(initial, accounts, transactions, seed));
  }
};

};