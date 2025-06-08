# Programs

Example programs written for my thesis, Behaviour Oriented Concurrency in Practice. My work can be found in "/jake/".

The runtime "benchmarker.cpp" is adapted directly from the Savina benchmarking suite [https://github.com/ic-slurp/verona-benchmarks/tree/main/savina].

The "ninja" build suite is required for running these programs.


For leader election examples:
Change the "--servers" flag to set the number of servers for each simulation.
Change the "--divisions" flag to set the number of starting servers (rings), maximum number of children per node (trees), number of extra edges (arbitrary).

For breakfast examples:
For breakfast_ideal, set bacon and eggs using "--bacon" and "--eggs" flags respectively.
breakfast has no flags to set.

