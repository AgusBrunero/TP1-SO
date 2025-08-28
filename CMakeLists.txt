cmake_minimum_required(VERSION 3.22)
project(untitled C)

set(CMAKE_C_STANDARD 99)

add_executable(master src/master.c src/defs.h)
add_executable(player src/player.c src/defs.h)
add_executable(view src/view.c src/defs.h)

set_target_properties(master player view PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
)