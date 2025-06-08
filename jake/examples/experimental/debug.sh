#! /bin/sh

cmake -B build -G Ninja -DCMAKE_MAKE_PROGRAM=$PWD/ninja/ninja -DCMAKE_BUILD_TYPE=Debug
cd build; 
../ninja/ninja && gdb --args ./jake/benchmarker --leader_ring_onlogn --servers 10