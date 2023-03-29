cmake_minimum_required(VERSION 3.18.0)

project(verona-savina CXX)

include(FetchContent)

FetchContent_Declare(
  verona
  GIT_REPOSITORY https://github.com/microsoft/verona-rt
  GIT_TAG        4c8bef6929b662763d2a98cb8462e953b02a97dd
  SOURCE_SUBDIR  src
)

FetchContent_MakeAvailable(verona)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(savina)

# add_subdirectory(computer-language-benchmarks-game)
