# /W3 warning in msvc fixed with 3.15
cmake_minimum_required(VERSION 3.15 FATAL_ERROR)


project(mkpoker
    VERSION 0.1.0
    LANGUAGES CXX
    DESCRIPTION "A Texas Holdem poker framework written in C++ 20.")


if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
    message(FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there.")
endif()


# configuration for dev, only executes if this is the main project
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
endif()


# base library, header only
add_library(mkpoker-mkp INTERFACE)
add_library(mkpoker::mkp ALIAS mkpoker-mkp)
target_include_directories(mkpoker-mkp INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>)
target_include_directories(mkpoker-mkp SYSTEM INTERFACE $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include>)
target_compile_features(mkpoker-mkp INTERFACE cxx_std_20)
target_compile_options(mkpoker-mkp INTERFACE $<$<CXX_COMPILER_ID:MSVC>:/W4 /permissive- /std:c++latest>)
target_compile_options(mkpoker-mkp INTERFACE $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -Wpedantic>)
target_compile_options(mkpoker-mkp INTERFACE $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -Wpedantic>)


# tests etc., execute only if this is the main project
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)

    # build tests
    message(STATUS "Testing enabled")
    enable_testing()
    add_subdirectory(test)

endif()
