# Concurrency
## Banking

### Actors
* The benchmark is constructed of a `Teller` actor and a number of `Account` actors.

* The `Teller` is responsible for spawning transactions between accounts and tracking how many transactions have completed.
* A transaction conists of:
  * Invoking a `credit` behaviour on an `Account`, `src`, with another account, `dst`, as an argument to send money to.
  * `src` invoking `debit` on `dst`
  * `dst` invoking `reply` on `src` to confirm the transaction has been complete.
  * `src` invoking `reply` on the `Teller` to make a completed transaction.
    * I think `credit` and `debit` in the original benchmark have confusing meanings,`credit` removes money from the receiving account and `debit` adds money to the receiving account.
* If an `Account` is processing a transaction and either `credit` or `debit` behaviours are invoked, then the `Account` stashes a token to allow the behaviour to be processed once the ongoing transaction has finished.

### BoC
* The benchmark has `Teller` and `Account` cowns.

* The `Teller` spawns a number of behaviours that grab random (but not aliasing) `src` and `dst` cowns at once, transfers money from one to the other, and informs the teller that another transaction has completed.

* We removed the (sort of) two phase commit that existed in the original benchmark, thus we need fewer messages and we do not need to stash and unstash behaviours.

## Barber

## Bounded Buffer

## Cigarette Smokers

## Concurrent Dictionary

## Logmap

## Philosophers

# Micro
## Fibonacci

# Parallel

TBD