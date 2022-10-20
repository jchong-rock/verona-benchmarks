cmake_minimum_required(VERSION 3.18.0)

project(verona-savina CXX)

include(FetchContent)

FetchContent_Declare(
  verona
  GIT_REPOSITORY https://github.com/microsoft/verona-rt
  GIT_TAG        aa1cab8c9c7cbc1e55e11d0464c0ea38ff9fe6d1
  SOURCE_SUBDIR  src
)

FetchContent_MakeAvailable(verona)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(savina)

# add_subdirectory(computer-language-benchmarks-game)
