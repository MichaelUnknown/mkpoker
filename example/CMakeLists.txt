##########################################
## exmaples ##
##########################################

# /W3 warning in msvc fixed with 3.15
cmake_minimum_required(VERSION 3.15 FATAL_ERROR)

# demo gamestate
add_executable(demo_gamestate demo_gamestate.cpp)
target_link_libraries(demo_gamestate PRIVATE ${PROJECT_NAMESPACE}::${PROJECT_NAME})
add_executable(demo_game_w_cards demo_game_w_cards.cpp)
target_link_libraries(demo_game_w_cards PRIVATE ${PROJECT_NAMESPACE}::${PROJECT_NAME})

# demo equity calculator
add_executable(demo_equity_calc demo_equity_calc.cpp)
target_link_libraries(demo_equity_calc PRIVATE ${PROJECT_NAMESPACE}::${PROJECT_NAME})

# demo cfr
find_package(Threads REQUIRED)
add_executable(demo_cfr demo_cfr.cpp)
target_link_libraries(demo_cfr PRIVATE ${PROJECT_NAMESPACE}::${PROJECT_NAME} Threads::Threads)

# demo print hand history
add_executable(demo_print_hh demo_print_hh.cpp)
target_link_libraries(demo_print_hh PRIVATE ${PROJECT_NAMESPACE}::${PROJECT_NAME})

# demo buckets
add_executable(demo_buckets demo_buckets.cpp)
target_link_libraries(demo_buckets PRIVATE ${PROJECT_NAMESPACE}::${PROJECT_NAME})
