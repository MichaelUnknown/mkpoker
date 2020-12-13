##########################################
## main library ##
##########################################

# /W3 warning in msvc fixed with 3.15
cmake_minimum_required(VERSION 3.15 FATAL_ERROR)


project(
    mkpoker
    VERSION 0.1.0
    LANGUAGES CXX
    DESCRIPTION "A Texas Holdem poker framework written in C++ 20."
)


# project options, these are off by default to not install gtest, propagate -Wpedantic etc.
option(MKPOKER_BUILD_EXAMPLES "Enable building the examples" OFF)
option(MKPOKER_BUILD_TESTS "Enable building the tests" OFF)
option(MKPOKER_BUILD_FOR_DEV "Use strict compiler warnings" OFF)
option(MKPOKER_ENABLE_CODE_COVERAGE "Enable test code coverage" OFF)


# do not allow building in-source
if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
    message(FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there.")
endif()


# dependencies
include(cmake/CPM.cmake)
# PackageProject.cmake will be used to make our target installable
CPMAddPackage(
    NAME PackageProject.cmake
    GITHUB_REPOSITORY TheLartians/PackageProject.cmake
    VERSION 1.3
)


# base library, header only
add_library(mkpoker INTERFACE)
set(PROJECT_LIBRARY_NAMESPACE "mkpoker")
add_library(${PROJECT_LIBRARY_NAMESPACE}::mkp ALIAS mkpoker)
target_include_directories(
    mkpoker
    INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include/${PROJECT_NAME}-${PROJECT_VERSION}>
)
target_compile_features(mkpoker INTERFACE cxx_std_20)
target_compile_options(mkpoker INTERFACE $<$<CXX_COMPILER_ID:MSVC>:/permissive->)


# configure according to options, build examples, tests etc.
if(MKPOKER_BUILD_FOR_DEV)
    target_compile_options(mkpoker INTERFACE $<$<CXX_COMPILER_ID:MSVC>:/W4>)
    target_compile_options(mkpoker INTERFACE $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -Wpedantic>)
    target_compile_options(mkpoker INTERFACE $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -Wpedantic>)
endif()
if(MKPOKER_BUILD_EXAMPLES)
    message(STATUS "Building samples enabled")
    add_subdirectory(example)
endif()
if(MKPOKER_BUILD_TESTS)
    message(STATUS "Testing enabled")
    enable_testing()
    add_subdirectory(test)
endif()


# make installable target
string(TOLOWER ${PROJECT_NAME}/version.h VERSION_HEADER_LOCATION)
packageProject(
    NAME ${PROJECT_NAME}
    VERSION ${PROJECT_VERSION}
    BINARY_DIR ${PROJECT_BINARY_DIR}
    INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include
    INCLUDE_DESTINATION include/${PROJECT_NAME}-${PROJECT_VERSION}
    VERSION_HEADER "${VERSION_HEADER_LOCATION}"
    DEPENDENCIES ""
)
