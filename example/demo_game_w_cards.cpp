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

int main()
{
    std::cout << "mkpoker game demo\nTo exit the program, type ':q' at any time\n\n";

    std::size_t num_players{};
    std::string input{};

    for (;;)
    {
        auto game = mkp::gamestate<6>(10000);

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
                return EXIT_SUCCESS;
            }

            const unsigned num_action = std::atoi(input.c_str());
            if (num_action >= 0 && num_action < vec_actions.size())
            {
                game.execute_action(vec_actions[num_action]);
                if (game.in_terminal_state())
                {
                    std::cout << "\nThe game reached showdown (or all but one player folded).\n\nNew game...\n";
                }
            }
        }
    }
}