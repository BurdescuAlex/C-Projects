cmake_minimum_required(VERSION 3.15)
project(Shelly C)

set(CMAKE_C_STANDARD 99)

add_executable(Shelly main.c console_input.c console_input.h input_parser.c input_parser.h action.c action.h meta_action.c meta_action.h)

target_link_libraries(Shelly readline)