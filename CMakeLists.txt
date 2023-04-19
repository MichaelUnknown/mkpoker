##########################################
## main library ##
##########################################

# /W3 warning in msvc fixed with 3.15
cmake_minimum_required(VERSION 3.15 FATAL_ERROR)


project(
    mkpoker
    VERSION 0.2.2
    LANGUAGES CXX
    DESCRIPTION "A Texas Holdem poker framework written in C++ 20."
)


# project options, these are off by default to not install gtest, propagate -Wpedantic etc.
option(MKPOKER_BUILD_EXAMPLES "Enable building the examples" OFF)
option(MKPOKER_BUILD_TESTS "Enable building the tests" OFF)
option(MKPOKER_BUILD_FOR_DEV "Use strict compiler warnings" OFF)
option(MKPOKER_ENABLE_CODE_COVERAGE "Enable test code coverage" OFF)
option(MKPOKER_ENABLE_BUCKETS "Enable bucket header (pulls in Boost as dependency!)" OFF)
# VS Code C/C++ extension IntelliSense needs compile_commands.json for include paths
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


# do not allow building in-source
if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
    message(FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there.")
endif()



# add dependencies via CPM, see https://github.com/cpm-cmake/CPM.cmake for more info
include(cmake/CPM.cmake)
# until replace by std::format
CPMAddPackage(
    NAME fmt
    GIT_TAG 9.1.0
    GITHUB_REPOSITORY fmtlib/fmt
    OPTIONS "FMT_INSTALL YES"
)
# gather all used licenses
CPMAddPackage("gh:cpm-cmake/CPMLicenses.cmake@0.0.6")
# PackageProject.cmake will be used to make our project installable
CPMAddPackage("gh:TheLartians/PackageProject.cmake@1.10.0")


# base library, header only
set(PROJECT_NAMESPACE ${PROJECT_NAME})
add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(
    ${PROJECT_NAME}
    INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include/${PROJECT_NAME}-${PROJECT_VERSION}>
)
target_compile_options(${PROJECT_NAME} INTERFACE $<$<CXX_COMPILER_ID:MSVC>:/permissive->)
# todo: remove c++ latest for msvc when std::format is supported by /std::c++20 set via cmake (or fmt has support for zoned_time)
target_compile_features(${PROJECT_NAME} INTERFACE $<$<CXX_COMPILER_ID:Clang,GNU,AppleClang>:cxx_std_20>)
target_compile_options(${PROJECT_NAME} INTERFACE $<$<CXX_COMPILER_ID:MSVC>:/std:c++latest>)
# before: target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_20)
target_link_libraries(${PROJECT_NAME} INTERFACE fmt::fmt)
# add Boost if buckets enabled
if(MKPOKER_ENABLE_BUCKETS)
    message(STATUS "mkpoker: bucket header enabled")
    find_package(Boost 1.74 QUIET COMPONENTS serialization)
    if(NOT Boost_FOUND)
    # If not provided as system lib, use CPM.cmake to get Boost from the official repo
    message(STATUS "mkpoker buckets: Boost system lib NOT found")
    CPMAddPackage(
        NAME Boost
        GITHUB_REPOSITORY "boostorg/boost"
        GIT_TAG "boost-1.82.0"
        VERSION "1.82.0"
    )
    target_link_libraries(${PROJECT_NAME} INTERFACE Boost::serialization)
    else()
        message(STATUS "mkpoker buckets: Boost system lib found")
    endif()
else()
    message(STATUS "mkpoker: bucket header DISABLED")
endif()


# configure according to options, build examples, tests etc.
if(MKPOKER_BUILD_FOR_DEV)
    message(STATUS "mkpoker: strict compiler warnings enabled")
    target_compile_options(${PROJECT_NAME} INTERFACE $<$<CXX_COMPILER_ID:MSVC>:/W4>)
    target_compile_options(${PROJECT_NAME} INTERFACE $<$<CXX_COMPILER_ID:Clang,GNU,AppleClang>:-Wall -Wextra -Wpedantic>)
endif()
if(MKPOKER_BUILD_EXAMPLES)
    message(STATUS "mkpoker: building samples enabled")
    add_subdirectory(example)
else()
    message(STATUS "mkpoker: building samples DISABLED")
endif()
if(MKPOKER_BUILD_TESTS)
    message(STATUS "mkpoker: testing enabled")
    enable_testing()
    add_subdirectory(test)
else()
    message(STATUS "mkpoker: testing DISABLED")
endif()


# create licenses file to include into the repository
# assuming build directory `build`, create with `cmake --build build --target write-licenses`
cpm_licenses_create_disclaimer_target(
  write-licenses "${CMAKE_CURRENT_BINARY_DIR}/third_party.txt" "${CPM_PACKAGES}"
)


# make installable target
string(TOLOWER ${PROJECT_NAME}/version.h VERSION_HEADER_LOCATION)
packageProject(
    NAME ${PROJECT_NAME}
    VERSION ${PROJECT_VERSION}
    NAMESPACE ${PROJECT_NAMESPACE}
    BINARY_DIR ${PROJECT_BINARY_DIR}
    INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include
    INCLUDE_DESTINATION include/${PROJECT_NAME}-${PROJECT_VERSION}
    VERSION_HEADER "${VERSION_HEADER_LOCATION}"
    COMPATIBILITY SameMajorVersion
    DEPENDENCIES "fmt 9.1.0;"
    ARCH_INDEPENDENT YES
)
