set(BENCHMARKER ${CMAKE_CURRENT_SOURCE_DIR})

unset(SRC)
aux_source_directory(${BENCHMARKER} SRC)

target_compile_options(verona_rt INTERFACE -g -fno-omit-frame-pointer)

add_executable(benchmarker ${SRC} ${snmalloc_SOURCE_DIR}/src/snmalloc/override/new.cc)
target_link_libraries(benchmarker snmalloc verona_rt)