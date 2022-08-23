cmake_minimum_required(VERSION 3.10.0)

project(verona-savina CXX)

set(VERONA_RT_ONLY_HEADER_LIBRARY ON)

add_subdirectory(verona/src/rt)

add_subdirectory(savina/verona-savina)

add_subdirectory(computer-language-benchmarks-game)