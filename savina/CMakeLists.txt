set(SAVINA ${CMAKE_CURRENT_SOURCE_DIR})

unset(SRC)
aux_source_directory(${SAVINA} SRC)

target_compile_options(verona_rt INTERFACE -g -fno-omit-frame-pointer)

add_executable(savina ${SRC} ${snmalloc_SOURCE_DIR}/src/snmalloc/override/new.cc)
target_link_libraries(savina snmalloc verona_rt)

add_executable(savina-stats ${SRC} ${snmalloc_SOURCE_DIR}/src/snmalloc/override/new.cc)
target_link_libraries(savina-stats snmalloc verona_rt)
target_compile_definitions(savina-stats PUBLIC USE_SCHED_STATS)

add_executable(savina-sys ${SRC})
target_link_libraries(savina-sys verona_rt)
target_compile_definitions(savina-sys INTERFACE USE_SYSTEMATIC_TESTING)
