##########################################
## tests ##
##########################################

# cmake 3.14 for FetchContent
cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

option(MKPOKER_ENABLE_CODE_COVERAGE "Enable test code coverage" OFF)

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
    if (MKPOKER_ENABLE_CODE_COVERAGE)
        target_compile_options(${TESTNAME} PUBLIC -O0 -g -fprofile-arcs -ftest-coverage)
        target_link_options(${TESTNAME} PUBLIC -fprofile-arcs -ftest-coverage)
    endif()
    gtest_discover_tests(${TESTNAME})
endmacro()

# actual tests #
package_add_test(bitset_test bitset_test.cpp mkpoker::mkp)

package_add_test(suit_test suit_test.cpp mkpoker::mkp)
package_add_test(rank_test rank_test.cpp mkpoker::mkp)
package_add_test(card_test card_test.cpp mkpoker::mkp)

package_add_test(cardset_test cardset_test.cpp mkpoker::mkp)
package_add_test(hand_test hand_test.cpp mkpoker::mkp)
package_add_test(range_test range_test.cpp mkpoker::mkp)

package_add_test(holdem_eval_result_test holdem_eval_result_test.cpp mkpoker::mkp)
package_add_test(holdem_eval_test holdem_eval_test.cpp mkpoker::mkp)

package_add_test(game_test game_test.cpp mkpoker::mkp)


