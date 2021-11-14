cmake_minimum_required(VERSION 3.20)
project(cache_simulator)

set(CMAKE_CXX_STANDARD 17)

include_directories(src)

file(GLOB_RECURSE SOURCES "src/*.cpp")

set(SOURCES ${SOURCES})

add_executable(cache-simulator ${SOURCES})
