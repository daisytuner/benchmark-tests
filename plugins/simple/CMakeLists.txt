cmake_minimum_required(VERSION 3.26)

project(simple-plugin VERSION 0.0.1 DESCRIPTION "A simple plugin for docc")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT TARGET sdfg::sdfg)
    find_package(sdfglib CONFIG REQUIRED)
endif()

add_library(SimplePlugin SHARED
    plugin.cpp
    simple_node.cpp
)
target_link_libraries(SimplePlugin PRIVATE sdfg::sdfg sdfg::opt)
