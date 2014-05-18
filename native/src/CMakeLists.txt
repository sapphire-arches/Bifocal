cmake_minimum_required(VERSION 2.8)

project(bifocal)

add_definitions("-std=c99")
add_definitions("-Wall -Wextra")

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})

add_executable(cameragrab_test cameragrab.c cam.c)

target_link_libraries(cameragrab_test GL GLEW glut m)
