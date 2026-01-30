cmake_minimum_required(VERSION 3.16)
project(vcl_test)

option(VCL_BUILD_TEST "Build vcl test." OFF)

if (VCL_BUILD_TEST)
    find_package(Catch2 3 REQUIRED)
    
    file(GLOB_RECURSE vcl_test_files "./*/*.*")

    add_executable(vcl_test ${vcl_test_files})
    target_link_libraries(vcl_test PRIVATE vcl Catch2::Catch2WithMain)

    file(GLOB vcl_files "VCL/*.vcl")

    foreach(file ${vcl_files})
        get_filename_component(filename "${file}" NAME)
        configure_file("VCL/${filename}" "${CMAKE_CURRENT_BINARY_DIR}/VCL/${filename}" COPYONLY)
    endforeach()
endif()