set(SAVINA ${CMAKE_CURRENT_SOURCE_DIR})

set(CMAKE_CXX_FLAGS)

unset(SRC)
aux_source_directory(${SAVINA} SRC)
add_executable(savina ${SRC})
target_link_libraries(savina verona_rt)