
cmake_minimum_required(VERSION 3.22)
project(tetris42 VERSION 0.0.2)

add_executable(tetris42 tetris42.c)

add_subdirectory(raylib)

find_path(RAYLIB_DIR "raylib.h" HINTS raylib/src)
include_directories(${RAYLIB_DIR})
LIST(APPEND LIBS raylib)
target_link_libraries(tetris42 ${LIBS} )
