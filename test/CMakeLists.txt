##########################################
## tests ##
##########################################

# cmake 3.14 for FetchContent
# /W3 warning in msvc fixed with 3.15, also supports FetchContent
cmake_minimum_required(VERSION 3.15 FATAL_ERROR)


# add gtest
# for the moment, we use a gtest with improved cmake support (that is not merged with master yet)
# https://github.com/google/googletest/pull/3376
#CPMAddPackage("gh:MichaelUnknown/googletest#05eb4740d18f69cf17919c15f7c0d34da35b7f5e@1.10.x")
CPMAddPackage("gh:google/googletest#release-1.11.0")
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
package_add_test(bitset_test bitset_test.cpp ${PROJECT_NAMESPACE}::${PROJECT_NAME})

package_add_test(suit_test suit_test.cpp ${PROJECT_NAMESPACE}::${PROJECT_NAME})
package_add_test(rank_test rank_test.cpp ${PROJECT_NAMESPACE}::${PROJECT_NAME})
package_add_test(card_test card_test.cpp ${PROJECT_NAMESPACE}::${PROJECT_NAME})

package_add_test(cardset_test cardset_test.cpp ${PROJECT_NAMESPACE}::${PROJECT_NAME})
package_add_test(hand_test hand_test.cpp ${PROJECT_NAMESPACE}::${PROJECT_NAME})
package_add_test(range_test range_test.cpp ${PROJECT_NAMESPACE}::${PROJECT_NAME})

package_add_test(holdem_eval_result_test holdem_eval_result_test.cpp ${PROJECT_NAMESPACE}::${PROJECT_NAME})
package_add_test(holdem_eval_test holdem_eval_test.cpp ${PROJECT_NAMESPACE}::${PROJECT_NAME})

package_add_test(game_test game_test.cpp ${PROJECT_NAMESPACE}::${PROJECT_NAME})
