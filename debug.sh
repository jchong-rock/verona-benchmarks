#! /bin/sh

cmake -B build -G Ninja -DCMAKE_MAKE_PROGRAM=$PWD/ninja/ninja -DCMAKE_BUILD_TYPE=Debug
cd build; 
../ninja/ninja && ./jake/benchmarker --leader_dag_no_mailbox