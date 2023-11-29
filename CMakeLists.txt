cmake_minimum_required(VERSION 3.20)
project(DynamicTypeLib)

set(CMAKE_CXX_STANDARD 23)

file(GLOB_RECURSE SOURCES src/**.cpp)

include_directories(include)
add_library(DynamicTypeLib ${SOURCES})
