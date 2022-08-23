cmake_minimum_required(VERSION 3.10.0)

project(verona-savina CXX)

set(VERONA_RT_ONLY_HEADER_LIBRARY ON)

add_subdirectory(verona/src/rt)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(savina)

add_subdirectory(computer-language-benchmarks-game)