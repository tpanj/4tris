project(42tris)

cmake_minimum_required(VERSION 3.22)

add_executable(42tris 42tris.cc)

add_subdirectory(raylib)

find_path(RAYLIB_DIR "raylib.h" HINTS raylib/src)
include_directories(${RAYLIB_DIR})
LIST(APPEND LIBS raylib)
target_link_libraries(42tris ${LIBS} )
