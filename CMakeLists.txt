cmake_minimum_required(VERSION 3.20)
project(DynamicTypeLib)

set(CMAKE_CXX_STANDARD 23)

file(GLOB_RECURSE SOURCES src/**.cpp)

add_library(DynamicTypeLib STATIC ${SOURCES})
target_include_directories(DynamicTypeLib PUBLIC include)