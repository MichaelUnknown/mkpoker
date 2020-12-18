##########################################
## exmaples ##
##########################################

# /W3 warning in msvc fixed with 3.15
cmake_minimum_required(VERSION 3.15 FATAL_ERROR)

# demo game
add_executable(demo_gamestate demo_gamestate.cpp)
target_link_libraries(demo_gamestate PRIVATE ${PROJECT_NAMESPACE}::${PROJECT_NAME})
add_executable(demo_game_w_cards demo_game_w_cards.cpp)
target_link_libraries(demo_game_w_cards PRIVATE ${PROJECT_NAMESPACE}::${PROJECT_NAME})
