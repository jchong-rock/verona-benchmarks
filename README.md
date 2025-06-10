# Programs

Example programs written for my thesis, Behaviour Oriented Concurrency in Practice. My work can be found in "/jake/".

The runtime `benchmarker.cpp` is adapted directly from the Savina benchmarking suite [https://github.com/ic-slurp/verona-benchmarks/tree/main/savina].

The "ninja" build suite is required for running these programs.


## For leader election examples:
Change the `--servers` flag to set the number of servers for each simulation.
Change the `--divisions` flag to set the number of starting servers (__ring__), maximum number of children per node (__tree__), number of extra edges (__arbitrary__).
`leader_ring_boc` does not support multiple starts, thus the `--divisions` flag will be ignored.

## For breakfast examples:
For `breakfast_ideal`, set bacon and eggs using `--bacon` and `--eggs` flags respectively.
breakfast has no flags to set.

## Example usage:

`run.sh --leader_ring --servers 100 --divisions 30`
Runs `leader_ring.h` with 100 servers and 30 starters.
