cmake_minimum_required(VERSION 3.16)
project(vcl_example_cvcl)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(cvcl ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)
target_link_libraries(cvcl PRIVATE vcl)