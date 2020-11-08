##########################################
## tests ##
##########################################

# cmake 3.14 for FetchContent
cmake_minimum_required(VERSION 3.14.0)

# force shared crt
# todo: check if this is only needed on windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# get GTest
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        release-1.10.0
)
FetchContent_MakeAvailable(googletest)

# for gtest_discover_tests
include(GoogleTest)

macro(package_add_test TESTNAME TESTFILE)
    add_executable(${TESTNAME} ${TESTFILE})
    target_link_libraries(${TESTNAME} PRIVATE gtest gtest_main ${ARGN})
    gtest_discover_tests(${TESTNAME})
endmacro()

# check the status of compiler flags
message(STATUS "test compiler flags (debug): ${CMAKE_CXX_FLAGS_DEBUG}")
message(STATUS "test compiler flags (release): ${CMAKE_CXX_FLAGS_RELEASE}")

# mkpoker::base #
package_add_test(suit_test suit_test.cpp mkpoker::base)
package_add_test(rank_test rank_test.cpp mkpoker::base)
package_add_test(card_test card_test.cpp mkpoker::base)
