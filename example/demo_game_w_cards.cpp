/*

mkpoker - demo command line app that starts with a new game state and lets
the user choose between the different legal actions for each player
and step through a game

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

#include <mkpoker/game/game.hpp>
#include <mkpoker/util/array.hpp>
#include <mkpoker/util/card_generator.hpp>

#include <algorithm>    // std::rotate
#include <chrono>       // sleep 1s
#include <cstdlib>      // atoi
#include <iostream>     //
#include <random>       //
#include <thread>       // sleep 1s

int main()
{
    constexpr std::size_t c_num_players = 6;
    constexpr int c_starting_chips = 20000;
    std::string input{};
    std::mt19937 rng{1927};
    mkp::card_generator cgen{};

    // keep track of player chips
    std::array<int, c_num_players> chips{c_starting_chips, c_starting_chips, c_starting_chips,
                                         c_starting_chips, c_starting_chips, c_starting_chips};

    std::cout << "mkpoker gameplay demo\nTo exit the program, type ':q' at any time\n\n";

    for (uint8_t player_pos = 0;; player_pos = (player_pos + 1) % c_num_players)
    {
        // generate game/cards for a six player game
        const auto random_cards = cgen.generate_v(5 + 2 * c_num_players);
        const mkp::gamecards<c_num_players> gamecards(random_cards);
        auto game = mkp::gamestate<c_num_players, 0, 1>(chips);
        std::cout << "New hand started. Your position is " << std::to_string(player_pos) << " \n";

        for (;;)
        {
            std::cout << "\n" << game.str_state() << "\n";
            std::cout << "The active player is: " << std::to_string(game.active_player())
                      << (game.active_player() == player_pos ? " (this is you)" : "") << "\n";
            const auto vec_actions = game.possible_actions();

            // player action
            if (game.active_player() == player_pos)
            {
                using mkp::gb_gamestate_t;
                std::cout << "Your cards are: " << gamecards.m_hands[player_pos].str() << "\n";

                const int num_board_cards_to_print = [&]() {
                    switch (game.gamestate_v())
                    {
                        case mkp::gb_gamestate_t::FLOP_BET:
                            return 3;
                        case mkp::gb_gamestate_t::TURN_BET:
                            return 4;
                        case mkp::gb_gamestate_t::RIVER_BET:
                            return 5;
                        default:
                            return 0;
                    }
                }();
                if (num_board_cards_to_print > 0)
                {
                    std::cout << "The board is: ";
                    for (const auto cards = gamecards.board_n(num_board_cards_to_print); auto&& c : cards)
                    {
                        std::cout << c.str() << " ";
                    }
                    std::cout << "\n";
                }
                std::cout << "\n";

                std::cout << "Please select any of the possible actions:\n";
                for (std::size_t i = 0; auto&& a : vec_actions)
                {
                    std::cout << "[" << i << "] " << a.str() << " ";
                    if ((i + 1) % 6 == 0)
                    {
                        std::cout << "\n";
                    }
                    ++i;
                }
                std::cout << "\n";

                for (;;)
                {
                    std::cin >> input;

                    if (input == ":q")
                    {
                        return EXIT_SUCCESS;
                    }
                    const unsigned num_action = std::atoi(input.c_str());
                    if (num_action < vec_actions.size())
                    {
                        game.execute_action(vec_actions[num_action]);
                        break;
                    }
                    else
                    {
                        std::cout << "invalid input...";
                    }
                }
            }
            else
            // opponents: select random actions
            {
                std::uniform_int_distribution<> dist_actions(0, static_cast<int>(vec_actions.size() - 1));
                const auto action = vec_actions[dist_actions(rng)];
                std::cout << "Opponents action: " << action.str() << "\n";
                std::this_thread::sleep_for(std::chrono::milliseconds(2500));
                game.execute_action(action);
            }

            // check if hand/round finished
            if (game.in_terminal_state())
            {
                std::cout << "\nThe hand ended.\n" << game.str_state() << "\n";
                const auto pots = game.all_pots();
                for (unsigned int i = 1; auto&& e : pots)
                {
                    std::cout << "Pot " << i << ":\nEligible players: ";
                    for (auto&& p : std::get<0>(e))
                    {
                        std::cout << p << " (" << gamecards.m_hands[p].str() << ") ";
                    }
                    std::cout << "\nThe board is: ";
                    for (const auto cards = gamecards.board_n(5); auto&& c : cards)
                    {
                        std::cout << c.str() << " ";
                    }
                    std::cout << "\nlower: " << std::get<2>(e) << ", upper: " << std::get<1>(e) << "\n";
                    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
                    ++i;
                }

                std::cout << "\nResults:\n";
                const auto results = game.is_showdown() ? game.payouts_showdown(gamecards) : game.payouts_noshowdown();
                for (unsigned int i = 0; i < results.size(); ++i)
                {
                    std::cout << i << ": " << results[i] << " (started with: " << chips[i] << " => " << chips[i] + results[i] << ")\n";
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(4000));

                {
                    using namespace mkp;
                    chips += results;
                }

                // if any player has less than 1BB, restock each players chips 20BB
                if (const auto found = std::find_if(chips.cbegin(), chips.cend(), [](const int p_chips) { return p_chips < 1000; });
                    found != chips.cend())
                {
                    for (unsigned int i = 0; i < chips.size(); ++i)
                    {
                        chips[i] += c_starting_chips;
                    }
                    std::cout << "Player" << std::distance(chips.cbegin(), found) << " dropped below 1BB. Every player received 20BB.\n";
                }

                // each player moves up one seat the next round
                std::rotate(chips.rbegin(), chips.rbegin() + 1, chips.rend());

                std::cout << "\n\n\n";
                break;
            }
        }
    }
}