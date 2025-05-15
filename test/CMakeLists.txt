cmake_minimum_required(VERSION 3.16)
project(vcl_test)

option(VCL_BUILD_TEST "Build vcl test." OFF)

if (VCL_BUILD_TEST)
    find_package(Catch2 3 REQUIRED)

    add_executable(vcl_tests "test.cpp")
    target_link_libraries(vcl_tests PRIVATE vcl Catch2::Catch2WithMain)

    file(GLOB vcl_files "vcl/*.vcl")

    foreach(file ${vcl_files})
        get_filename_component(filename "${file}" NAME)
        configure_file("vcl/${filename}" "${CMAKE_CURRENT_BINARY_DIR}/vcl/${filename}" COPYONLY)
    endforeach()
endif()