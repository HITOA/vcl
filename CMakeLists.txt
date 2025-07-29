cmake_minimum_required(VERSION 3.16)
project(vcl)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

add_library(vcl 
    STATIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Aggregate.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Callable.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/CallableTemplate.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ExecutionContext.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Handle.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Intrinsic.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Intrinsics.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ModuleBuilder.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ModuleContext.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ModuleOptimizer.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ModuleVerifier.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Scope.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/String.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/StructDefinition.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/StructTemplate.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/TemplateUtils.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Type.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Utility.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Value.hpp

        ${CMAKE_CURRENT_SOURCE_DIR}/src/Aggregate.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Attribute.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Callable.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/CallableTemplate.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Debug.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Directive.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ExecutionContext.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ExecutionSession.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Intrinsic.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Intrinsics.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Lexer.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Meta.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Module.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ModuleBuilder.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ModuleContext.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ModuleInfo.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ModuleOptimizer.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ModuleVerifier.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/NativeTarget.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Parser.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/PrettyPrinter.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Scope.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Source.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/String.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/StructDefinition.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/StructTemplate.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/TemplateUtils.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Type.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Value.cpp)
target_include_directories(vcl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_property(TARGET vcl PROPERTY COMPILE_WARNING_AS_ERROR ON)

llvm_map_components_to_libnames(llvm_libs support core irreader orcjit nativecodegen)

target_link_libraries(vcl ${llvm_libs})

target_include_directories(vcl PRIVATE ${LLVM_INCLUDE_DIRS})

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/examples")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/test")

option(VCL_BUILD_DOC "Build VCL documentation" OFF)

if (VCL_BUILD_DOC)
    find_package(Doxygen)
    if (DOXYGEN_FOUND)
        set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
        set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
        configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
        message("Doxygen build started")
        add_custom_target(vcl_doc_doxygen ALL
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM )

    else (DOXYGEN_FOUND)
        message("Doxygen need to be installed to generate the doxygen documentation")
    endif (DOXYGEN_FOUND)
endif (VCL_BUILD_DOC)