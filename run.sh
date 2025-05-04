#! /bin/sh

cmake -B build -G Ninja -DCMAKE_MAKE_PROGRAM=$PWD/ninja/ninja -DCMAKE_BUILD_TYPE=Release
cd build; 
../ninja/ninja && ./jake/benchmarker --leader_dag_no_mailbox --servers 20 --divisions 2