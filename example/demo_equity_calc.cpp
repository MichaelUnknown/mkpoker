/*

mkpoker - demo command line app that calculates equity for different hands

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

#include <mkpoker/base/card.hpp>
#include <mkpoker/base/hand.hpp>
#include <mkpoker/holdem/holdem_equity_calculation.hpp>
#include <mkpoker/holdem/holdem_evaluation.hpp>

#include <vector>

#include <fmt/core.h>
#include <fmt/format.h>

void pretty_print(const std::vector<mkp::hand_2c>& hands, const mkp::equity_calculation_result_t& results,
                  const std::vector<mkp::card>& vec_board = {})
{
    if (vec_board.size() > 0)
    {
        std::string str;
        for (auto&& card : vec_board)
        {
            str.append(card.str());
        }
        fmt::print(" Board: {}\n", str);
    }
    fmt::print(" Hand |  Equity |    Wins |    Ties \n");
    fmt::print("------+---------+---------+---------\n");
    for (unsigned i = 0; i < hands.size(); ++i)
    {
        fmt::print(" {} | {:>6}% | {:>7} | {:>7}\n", hands[i].str(), fmt::format("{:05.2f}", results.m_equities[i]), results.m_wins[i],
                   results.m_ties[i]);
    }
}

int main()
{
    // hole cards
    const mkp::hand_2c h1{"AcAd"};
    const mkp::hand_2c h2{"Th9h"};

    ////////////////////////////////////////////////////////////////////////////
    // preflop calculation, no board
    ////////////////////////////////////////////////////////////////////////////
    const std::vector<mkp::hand_2c> vec_hands{h1, h2};
    const auto results_pf = mkp::calculate_equities(vec_hands);
    pretty_print(vec_hands, results_pf);
    fmt::print("\n\n");

    ////////////////////////////////////////////////////////////////////////////
    // board with one or two cards
    ////////////////////////////////////////////////////////////////////////////
    std::vector<mkp::card> board{mkp::card{"Ts"}};
    const auto results_board_test1 = mkp::calculate_equities(vec_hands, board);
    pretty_print(vec_hands, results_board_test1, board);
    fmt::print("\n\n");

    board.push_back(mkp::card{"9s"});    // Ts9s
    const auto results_board_test2 = mkp::calculate_equities(vec_hands, board);
    pretty_print(vec_hands, results_board_test2, board);
    fmt::print("\n\n");

    ////////////////////////////////////////////////////////////////////////////
    // flop
    ////////////////////////////////////////////////////////////////////////////
    board.push_back(mkp::card{"Tc"});    // Ts9sTc
    const auto results_board_flop = mkp::calculate_equities(vec_hands, board);
    pretty_print(vec_hands, results_board_flop, board);
    fmt::print("\n\n");

    ////////////////////////////////////////////////////////////////////////////
    // turn
    ////////////////////////////////////////////////////////////////////////////
    board.push_back(mkp::card{"9c"});    // Ts9sTc9c
    const auto results_board_turn = mkp::calculate_equities(vec_hands, board);
    pretty_print(vec_hands, results_board_turn, board);
    fmt::print("\n\n");

    ////////////////////////////////////////////////////////////////////////////
    // river
    ////////////////////////////////////////////////////////////////////////////
    board.push_back(mkp::card{"Ah"});    // Ts9sTc9cAh
    const auto results_board_river = mkp::calculate_equities(vec_hands, board);
    pretty_print(vec_hands, results_board_river, board);
    fmt::print("\n\n");

    ////////////////////////////////////////////////////////////////////////////
    // preflop with more players
    ////////////////////////////////////////////////////////////////////////////
    const mkp::hand_2c h3{"8c8s"};
    const mkp::hand_2c h4{"2d7h"};
    const std::vector<mkp::hand_2c> vec_hands_4p{h1, h2, h3, h4};
    const auto results_pf_4p = mkp::calculate_equities(vec_hands_4p);
    pretty_print(vec_hands_4p, results_pf_4p);
    fmt::print("\n\n");

    return EXIT_SUCCESS;
}