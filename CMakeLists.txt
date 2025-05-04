cmake_minimum_required(VERSION 3.18.0)

project(verona-savina CXX)

include(FetchContent)

FetchContent_Declare(
  verona
  GIT_REPOSITORY https://github.com/microsoft/verona-rt
  GIT_TAG        19c01dff5e6abc60d8c56998c66fbfa8f2f9be81
  SOURCE_SUBDIR  src
)

FetchContent_MakeAvailable(verona)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(jake)

# add_subdirectory(computer-language-benchmarks-game)
