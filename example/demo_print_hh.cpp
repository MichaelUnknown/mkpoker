/*

mkpoker - demo command line app that prints played hands

Copyright (C) Michael Knörzer

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
    std::uniform_int_distribution<> distrib(70, 130);
    std::mt19937 rng{1927};
    mkp::card_generator cgen{};

    constexpr std::size_t c_num_players_game_6 = 6;
    uint64_t hand_id = 231000111000;    // arbitrary hand id for hand history
    std::FILE* f_hh = std::fopen("hh01.txt", "w");
    int cnt = 0;

    // create some random hands and log them
    for (;;)
    {
        // generate chips, game & cards for a six player game
        std::array<std::string, c_num_players_game_6> names = {"Alf", "Bert", "Charles", "Dave", "Ethan", "Fred"};
        // create random starting chips between 70 and 130 BBs
        const auto chips = mkp::make_array<int, c_num_players_game_6>([&](auto) { return 1000 * distrib(rng); });
        const auto random_cards = cgen.generate_v(5 + 2 * c_num_players_game_6);
        const mkp::gamecards<c_num_players_game_6> gamecards(random_cards);
        auto game = mkp::gamestate<c_num_players_game_6, 50, 100>(chips);

        // create hh printer, $1.00 <=> 50'000 mBB, pov of position 2
        auto hh_printer = mkp::hh_ps(game, gamecards, names, f_hh, 2, 50'000, hand_id);
        fmt::print("game {}\n", cnt);

        // play out that hand with random actions
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
            std::fclose(f_hh);
            break;
        }

        ++hand_id;
    }

    constexpr std::size_t c_num_players_game_3 = 3;
    f_hh = std::fopen("hh02.txt", "w");
    cnt = 0;

    // create some random hands and log them
    for (;;)
    {
        // generate chips, game & cards for a six player game
        std::array<std::string, c_num_players_game_3> names = {"Alf", "Bert", "Charles"};
        // create random starting chips between 70 and 130 BBs
        const auto chips = mkp::make_array<int, c_num_players_game_3>([&](auto) { return 1000 * distrib(rng); });
        const auto random_cards = cgen.generate_v(5 + 2 * c_num_players_game_3);
        const mkp::gamecards<c_num_players_game_3> gamecards(random_cards);
        auto game = mkp::gamestate<c_num_players_game_3, 5, 100>(chips);

        // create hh printer, $1.00 <=> 50'000 mBB, pov of position 2
        auto hh_printer = mkp::hh_ps(game, gamecards, names, f_hh, 2, 50'000, hand_id);
        fmt::print("game {}\n", cnt);

        // play out that hand with random actions
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
            std::fclose(f_hh);

            return EXIT_SUCCESS;
        }

        ++hand_id;
    }
}