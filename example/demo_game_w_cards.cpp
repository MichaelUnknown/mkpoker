/*
mkpoker - demo cli app that starts with a new game state and lets
the user choose between the different legal actions for each player
and step through a game

Copyright (C) 2020 Michael Knörzer

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as
published by the Free Software Foundation, either version 3 of the
License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <mkpoker/game/game.hpp>

#include <cstdlib>
#include <iostream>
#include <variant>

using g2 = mkp::gamestate<2>;
using g3 = mkp::gamestate<3>;
using g4 = mkp::gamestate<4>;
using g5 = mkp::gamestate<5>;
using g6 = mkp::gamestate<6>;

int main()
{
    std::cout << "mkpoker game demo\nTo exit the program, type ':q' at any time\n\n";

    std::size_t num_players{};
    std::string input{};

    for (;;)
    {
        std::cout << "Please specify the number of players(2..6)\n ";
        for (;;)
        {
            std::cin >> input;

            if (input == ":q")
            {
                return EXIT_SUCCESS;
            }

            num_players = std::atoi(input.c_str());

            if (num_players >= 2 && num_players <= 6)
            {
                std::cout << "selected " << num_players << " players\n";
                break;
            }
        }

        std::variant<std::monostate, g2, g3, g4, g5, g6> game;

        switch (num_players)
        {
            case 2:
                game = mkp::gamestate<2>(10000);
                break;
            case 3:
                game = mkp::gamestate<3>(10000);
                break;
            case 4:
                game = mkp::gamestate<4>(10000);
                break;
            case 5:
                game = mkp::gamestate<5>(10000);
                break;
            case 6:
                game = mkp::gamestate<6>(10000);
                break;
        }

        const auto cont = std::visit(
            [&input](auto&& game) -> bool {
                if constexpr (!std::is_same_v<std::decay_t<decltype(game)>, std::monostate>)
                {
                    for (;;)
                    {
                        std::cout << "\n" << game.str_state() << "\n";
                        std::cout << "The active player is: " << std::to_string(game.active_player()) << "\n";
                        std::cout << "Please select any of the possible actions:\n\n";

                        const auto vec_actions = game.possible_actions();
                        for (std::size_t i = 0; auto&& a : vec_actions)
                        {
                            std::cout << "[" << i << "] " << a.str() << "\n";
                            ++i;
                        }

                        std::cin >> input;

                        if (input == ":q")
                        {
                            // do not continue
                            false;
                        }

                        const unsigned num_action = std::atoi(input.c_str());
                        if (num_action >= 0 && num_action < vec_actions.size())
                        {
                            game.execute_action(vec_actions[num_action]);
                            if (game.in_terminal_state())
                            {
                                std::cout << "\nThe game reached showdown (or all but one player folded).\n\nNew game...\n";

                                // do continue
                                return true;
                            }
                        }
                    }
                }
                else
                {
                    return false;
                }
            },
            game);
        if (!cont)
        {
            return EXIT_SUCCESS;
        }
    }
}