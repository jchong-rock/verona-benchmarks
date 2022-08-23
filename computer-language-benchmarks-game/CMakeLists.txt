set(BENCHMARKS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/benchmarks)
subdirlist(BENCHMARKS ${BENCHMARKS_DIR})

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

foreach(BENCHMARK ${BENCHMARKS})
  unset(SRC)
  aux_source_directory(${BENCHMARKS_DIR}/${BENCHMARK} SRC)
  add_executable(${BENCHMARK} ${SRC})
  target_link_libraries(${BENCHMARK} verona_rt)
endforeach()