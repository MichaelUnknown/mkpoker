/*

mkpoker - demo command line app that prints played hands

Copyright (C) 2021 Michael Kn�rzer

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

        // create hh printer, 1BB <=> 1000, pov of position 2
        auto hh_printer = mkp::hh_ps(game, gamecards, names, f_hh, 2, 1000);
        //mkp::hh_ps<mkp::gamestate, 6> test1 = {};
        //mkp::hh_ps<mkp::gamestate, 6> test{game, gamecards, names, f_hh, 1, 2};

        for (;;)
        {
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
                //for (unsigned int i = 1; auto&& e : pots)
                //{
                //    std::cout << "Pot " << i << ":\nEligible players: ";
                //    for (auto&& p : std::get<0>(e))
                //    {
                //        std::cout << p << " (" << gamecards.m_hands[p].str() << ") ";
                //    }
                //    std::cout << "\nThe board is: ";
                //    for (const auto cards = gamecards.board_n(5); auto&& c : cards)
                //    {
                //        std::cout << c.str() << " ";
                //    }
                //    std::cout << "\nlower: " << std::get<2>(e) << ", upper: " << std::get<1>(e) << "\n";
                //    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
                //    ++i;
                //}

                //std::cout << "\nResults:\n";
                //const auto results = game.is_showdown() ? game.payouts_showdown(gamecards) : game.payouts_noshowdown();
                //for (unsigned int i = 0; i < results.size(); ++i)
                //{
                //    std::cout << i << ": " << results[i] << " (started with: " << chips[i] << " => " << chips[i] + results[i] << ")\n";
                //}
                //std::this_thread::sleep_for(std::chrono::milliseconds(4000));

                std::rotate(names.begin(), names.begin() + 1, names.end());
                ++cnt;
                break;
            }
        }

        std::fflush(f_hh);

        if (cnt > 10)
        {
            return EXIT_SUCCESS;
        }
    }

    std::fclose(f_hh);
}