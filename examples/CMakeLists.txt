cmake_minimum_required(VERSION 3.16)
project(vcl_examples)

option(VCL_BUILD_EXAMPLES "Build vcl examples." ON)

if (VCL_BUILD_EXAMPLES)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/cvcl)
endif()