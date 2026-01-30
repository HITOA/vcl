cmake_minimum_required(VERSION 3.16)
project(vcl_test)

option(VCL_BUILD_CVCL "Build cvcl." OFF)

if (VCL_BUILD_CVCL)
    add_executable(vcl_cvcl main.cpp)
    target_link_libraries(vcl_cvcl PRIVATE vcl)
endif()