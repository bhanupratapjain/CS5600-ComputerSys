cmake_minimum_required(VERSION 3.6)
project(CS5600_ComputerSys)

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror")

#set_target_properties(CS5600_ComputerSys PROPERTIES LINKER_LANGUAGE c)

set(SOURCE_FILES main.cpp)
set(SOURCE_FILES hw2/malloc.c)
set(SOURCE_FILES hw2/malloc.c)
set(SOURCE_FILES hw2/test1.c)
set(SOURCE_FILES hw2/t-test1.c)
add_executable(CS5600_ComputerSys ${SOURCE_FILES})