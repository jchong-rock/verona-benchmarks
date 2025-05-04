set(BENCHMARKER ${CMAKE_CURRENT_SOURCE_DIR})

unset(SRC)
aux_source_directory(${BENCHMARKER} SRC)

target_compile_options(verona_rt INTERFACE -g -fno-omit-frame-pointer)

add_executable(benchmarker ${SRC} ${snmalloc_SOURCE_DIR}/src/snmalloc/override/new.cc)
target_link_libraries(benchmarker snmalloc verona_rt)

add_executable(benchmarker-stats ${SRC} ${snmalloc_SOURCE_DIR}/src/snmalloc/override/new.cc)
target_link_libraries(benchmarker-stats snmalloc verona_rt)
target_compile_definitions(benchmarker-stats PUBLIC USE_SCHED_STATS)

add_executable(benchmarker-sys ${SRC})
target_link_libraries(benchmarker-sys verona_rt)
target_compile_definitions(benchmarker-sys INTERFACE USE_SYSTEMATIC_TESTING)
