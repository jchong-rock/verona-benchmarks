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

### Changes

* We removed the (sort of) two phase commit that existed in the original benchmark, thus we need fewer messages and we do not need to stash and unstash behaviours.

## Barber

This one is quite confusing, I need to come back to it with fresh eyes. There is some give and take between the problem and the benchmark, and what is the sweetspot.

### Actors

* The sleeping barber problem can be found [here](https://en.wikipedia.org/wiki/Sleeping_barber_problem).

* The benchmark has a `WaitingRoom`, `Barber` and `CustomerFactory` actor and many `Customer` actors.

* The `WaitingRoom` manages the available seats, tells customer to leave if they are all full, wait if the `Barber` is busy and tells the `Barber` of a waiting customer.

* This requires state on the `WaitingRoom` to avoid giving the `Barber` multiple customers at once, and for the `Barber` to keep the `WaitingRoom` informed about whether they are busy or not.

* There seem to be some extra requirements that the `Barber` doesn't receive an `enter` message for a `Customer` until a `Customer` has been processed, which seems odd because one could just send `enter` messages and they would be atomically processed.

### BoC

* I have provided two versions; both strip a lot of the communication logic away so that there is essentially a core logic in the `WaitingRoom` that grabs multiple cowns together that need updating together.
  * Rather than sending a message from `WaitingRoom` to `Barber` to `Customer` to `WaitingRoom` etc. 
  * In one, the queue of available seats is now a counter and we only enqueue `size` behaviours on the `Barber` and use that as the seats
  * The second only enqueues a behaviour on the `Baber` when the `Barber` is neither working nor has a message enqueued.
  * The latter may be a more faithful translation, but it takes a lot more time as it creates a lot of messages where a customer trials and fails to join the waiting room. To investigate - the numbers seem so far apart in attempted entries though.

## Bounded Buffer

I'm not sure there is anything to do here, there is a centralized bounded channel that is read from and written to by independent consumers and producers.

I experimented a bit with replacing the central buffer of values with a buffer of pending producers, but that is only to construct a rendezvous between a producer and consumer that doesn't really need to happen.

### Actors

## Cigarette Smokers

There isn't any scope for BoC-like improvements to be made.

## Concurrent Dictionary

The only thing I can think to do here is to make the reads parallel but that seems to slow things down.

## Logmap

## Philosophers

# Actors

A central arbiter keeps track of which forks (which are represented by a vector of bools) are in use and tells philosophers whether they can eat.

# BoC

The arbiter is now a table to say everything is finished, but otherwise philosophers are told which forks (each of which is a cown) they want at the beginning of the program and try to grab them whenever they want to eat.

# Micro
## Fibonacci

# Actors

When solving the problem for `n`:
  * if `n=1` then return one
  * otherwise, the solver actor creates two child solver actors to solve for `n - 2` and `n - 1`, they solve their problem and send the response back to the parent, once the parent has both responses they are summed together. Respond to a parent if there is one.

# BoC

When solving for `n`:
  * if `n=1` return a cown containing `1`
  * otherwise, obtain the cowns with the solutions `n-1` and `n-2` and `when` on them sum the results into the `n-1` solution cown, also return the `n-1` solution cown.

# Parallel

TBD