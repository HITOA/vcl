cmake_minimum_required(VERSION 3.16)
project(vcl_example_test)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(vcltest ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)
target_link_libraries(vcltest PRIVATE vcl)