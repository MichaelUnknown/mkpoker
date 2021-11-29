/*

mkpoker - demo command line app that prints played hands

Copyright (C) 2021 Michael Knörzer

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

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <mkpoker/game/game.hpp>
#include <mkpoker/game/handhistory.hpp>
#include <mkpoker/util/array.hpp>
#include <mkpoker/util/card_generator.hpp>

#include <algorithm>    // std::rotate
#include <array>        //
#include <cstdio>       //
#include <random>       //

#include <fmt/core.h>

int main()
{
    int cnt = 0;

    constexpr std::size_t c_num_players = 6;
    std::uniform_int_distribution<> distrib(50'000, 100'000);
    std::string input{};
    std::mt19937 rng{1927};
    mkp::card_generator cgen{};
    std::FILE* f_hh = std::fopen("hh01.txt", "w");

    for (;;)
    {
        // generate chips, game & cards for a six player game
        std::array<std::string, 6> names = {"Alf", "Bert", "Charles", "Dave", "Ethan", "Fred"};
        // create random starting chips
        const auto chips = mkp::make_array<int, c_num_players>([&](auto) { return distrib(rng); });
        const auto random_cards = cgen.generate_v(5 + 2 * c_num_players);
        const mkp::gamecards<c_num_players> gamecards(random_cards);
        auto game = mkp::gamestate<c_num_players>(chips);

        // create hh printer, 1$ <=> 50'000 mBB, pov of position 2
        auto hh_printer = mkp::hh_ps(game, gamecards, names, f_hh, 2, 50'000);
        //mkp::hh_ps<mkp::gamestate, 6> test1 = {};
        //mkp::hh_ps<mkp::gamestate, 6> test{game, gamecards, names, f_hh, 1, 2};
        fmt::print("game {}\n", cnt);

        for (;;)
        {
            fmt::print("{}", game.str_state());
            const auto vec_actions = game.possible_actions();
            // select random actions

            std::uniform_int_distribution<> dist_actions(0, static_cast<int>(vec_actions.size() - 1));
            const auto action = vec_actions[dist_actions(rng)];
            fmt::print("player {} ({}) took action {}\n", game.active_player(), names[game.active_player()], action.str());
            game.execute_action(action);
            hh_printer.add_action(action);

            // check if hand/round finished
            if (game.in_terminal_state())
            {
                fmt::print("the hand ended\n{}\n", game.str_state());
                const auto pots = game.all_pots();

                std::rotate(names.begin(), names.begin() + 1, names.end());
                ++cnt;
                break;
            }
        }

        std::fflush(f_hh);

        if (cnt > 100)
        {
            return EXIT_SUCCESS;
        }
    }

    std::fclose(f_hh);
}