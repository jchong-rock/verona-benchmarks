#! /bin/sh

cmake -B build -G Ninja -DCMAKE_MAKE_PROGRAM=$PWD/ninja/ninja -DCMAKE_BUILD_TYPE=Release
cd build; 
../ninja/ninja && ./jake/benchmarker --breakfast --servers 100 --divisions 40