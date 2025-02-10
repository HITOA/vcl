cmake_minimum_required(VERSION 3.16)
project(vcl)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

add_library(vcl 
    STATIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ast.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/jit.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/jitbuiltins.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/jittype.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/lexer.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/parser.cpp)
target_include_directories(vcl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

llvm_map_components_to_libnames(llvm_libs support core irreader orcjit nativecodegen)

target_link_libraries(vcl ${llvm_libs})

target_include_directories(vcl PRIVATE ${LLVM_INCLUDE_DIRS})

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/examples)