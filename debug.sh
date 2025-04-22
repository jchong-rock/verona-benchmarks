#! /bin/sh

cmake -B build -G Ninja -DCMAKE_MAKE_PROGRAM=/homes/jpc21/verona-benchmarks/ninja/ninja -DCMAKE_BUILD_TYPE=Debug
cd build; 
../ninja/ninja && gdb --args ./savina/savina --leader