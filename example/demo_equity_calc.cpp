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

#include <mkpoker/base/hand.hpp>
#include <mkpoker/holdem/holdem_evaluation.hpp>
#include <mkpoker/util/card_generator.hpp>

#include <algorithm>    // std::max_element, std::count
#include <numeric>      // std::reduce
#include <stdexcept>    // std::runtime_error
#include <vector>

#include <fmt/core.h>
#include <fmt/format.h>

struct calculation_result_t
{
    std::vector<uint32_t> m_wins;
    std::vector<uint32_t> m_ties;
    std::vector<float> m_equities;
};

void pretty_print(const std::vector<mkp::hand_2c>& hands, const calculation_result_t& results, const std::vector<mkp::card>& vec_board = {})
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

calculation_result_t calc_results(const std::vector<mkp::hand_2c>& hands, const std::vector<mkp::card>& vec_board = {})
{
    using namespace mkp::constants;

    if (const auto sz = hands.size(); sz < 2 || sz > 9)
    {
        throw std::runtime_error(fmt::format("invalid number of hands (should be in the range of [2,9], given {})", sz));
    }

    const auto all_hole_cards = [&]() {
        mkp::cardset cs{};
        for (auto&& hand : hands)
        {
            cs = cs.combine(hand.as_cardset());
        }
        return cs;
    }();

    if (all_hole_cards.size() != hands.size() * 2)
    {
        throw std::runtime_error(
            fmt::format("hands contain duplicate cards: number of unique cards ({}) is smaller than {} (2 * {}[number of hands])",
                        all_hole_cards.size(), hands.size() * 2, hands.size()));
    }

    if (vec_board.size() > 5)
    {
        throw std::runtime_error(
            fmt::format("invalid number of board cards (should be less or equal to five, given {})", vec_board.size()));
    }
    const auto board = [&]() {
        mkp::cardset cs{};
        for (auto&& card : vec_board)
        {
            cs.insert(card);
        }
        return cs;
    }();
    if (board.size() != vec_board.size())
    {
        throw std::runtime_error(
            fmt::format("board contains duplicate cards ({} cards given but only {} of them are unique)", vec_board.size(), board.size()));
    }
    const auto all_fixed_cards = board.combine(all_hole_cards);
    if (all_fixed_cards.size() != board.size() + all_hole_cards.size())
    {
        throw std::runtime_error(
            fmt::format("hands and board contain duplicate cards: number of unique cards ({}) is smaller than {} ({}[number of board "
                        "cards] + {}[number of cards in all hands])",
                        all_fixed_cards.size(), board.size() + all_hole_cards.size(), board.size(), all_hole_cards.size()));
    }

    std::vector<uint32_t> wins;
    std::vector<uint32_t> ties;
    std::vector<uint32_t> score;
    std::vector<float> equities;
    for (unsigned i = 0; i < hands.size(); ++i)
    {
        wins.push_back(0);
        ties.push_back(0);
        score.push_back(0);
        equities.push_back(0.0f);
    }

    // calculate wins / losses and store them
    auto calculate_and_store_results = [&](const mkp::cardset& additional_cards) {
        const auto runout = additional_cards.combine(board);
        std::vector<mkp::holdem_result> results;
        for (auto&& hand : hands)
        {
            results.emplace_back(mkp::evaluate_unsafe(runout.combine(hand.as_cardset())));
        }
        const auto it_max = std::max_element(results.cbegin(), results.cend());
        const auto num_max = std::count(results.cbegin(), results.cend(), *it_max);
        if (num_max > 1)
        {
            // more than one winner -> tie
            for (unsigned n = 0; n < results.size(); ++n)
            {
                if (results[n] == *it_max)
                {
                    ties[n] += 1;
                    score[n] += 1;
                }
            }
        }
        else
        {
            // only one winner
            for (unsigned n = 0; n < results.size(); ++n)
            {
                if (results[n] == *it_max)
                {
                    wins[n] += 1;
                    score[n] += 2;
                    break;
                }
            }
        }
    };

    if (board.size() == 5)
    {
        // calculate immediately - we already have 5 cards
        calculate_and_store_results(mkp::cardset{});
    }
    else
    {
        for (uint8_t i = 0; i < c_deck_size; ++i)
        {
            mkp::card c1{i};
            if (all_fixed_cards.contains(c1))
            {
                continue;
            }

            if (board.size() == 4)
            {
                // calculate immediately - we already have 5 cards
                const auto additional_cards = mkp::cardset{c1};
                calculate_and_store_results(additional_cards);
            }
            else
            {
                for (uint8_t j = i + 1; j < c_deck_size; ++j)
                {
                    mkp::card c2{j};
                    if (all_fixed_cards.contains(c2))
                    {
                        continue;
                    }

                    if (board.size() == 3)
                    {
                        // calculate immediately - we already have 5 cards
                        const auto additional_cards = mkp::cardset{c1, c2};
                        calculate_and_store_results(additional_cards);
                    }
                    else
                    {
                        for (uint8_t k = j + 1; k < c_deck_size; ++k)
                        {
                            mkp::card c3{k};
                            if (all_fixed_cards.contains(c3))
                            {
                                continue;
                            }

                            if (board.size() == 2)
                            {
                                // calculate immediately - we already have 5 cards
                                const auto additional_cards = mkp::cardset{c1, c2, c3};
                                calculate_and_store_results(additional_cards);
                            }
                            else
                            {
                                for (uint8_t l = k + 1; l < c_deck_size; ++l)
                                {
                                    mkp::card c4{l};
                                    if (all_fixed_cards.contains(c4))
                                    {
                                        continue;
                                    }

                                    if (board.size() == 1)
                                    {
                                        // calculate immediately - we already have 5 cards
                                        const auto additional_cards = mkp::cardset{c1, c2, c3, c4};
                                        calculate_and_store_results(additional_cards);
                                    }
                                    else
                                    {
                                        // draw 5th card
                                        for (uint8_t m = l + 1; m < c_deck_size; ++m)
                                        {
                                            mkp::card c5{m};
                                            if (all_fixed_cards.contains(c5))
                                            {
                                                continue;
                                            }
                                            // calc and store results
                                            const auto additional_cards = mkp::cardset{c1, c2, c3, c4, c5};
                                            calculate_and_store_results(additional_cards);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // calc actual equity and return
    const auto total_score = std::reduce(score.cbegin(), score.cend(), uint32_t(0));
    for (unsigned i = 0; i < hands.size(); ++i)
    {
        equities[i] = static_cast<float>(score[i]) / total_score;
        equities[i] *= 100;
    }

    return calculation_result_t{wins, ties, equities};
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
    const auto results_pf = calc_results(vec_hands);
    pretty_print(vec_hands, results_pf);
    fmt::print("\n\n");

    ////////////////////////////////////////////////////////////////////////////
    // board with one or two cards
    ////////////////////////////////////////////////////////////////////////////
    std::vector<mkp::card> board{mkp::card{"Ts"}};
    const auto results_board_test1 = calc_results(vec_hands, board);
    pretty_print(vec_hands, results_board_test1, board);
    fmt::print("\n\n");

    board.push_back(mkp::card{"9s"});    // Ts9s
    const auto results_board_test2 = calc_results(vec_hands, board);
    pretty_print(vec_hands, results_board_test2, board);
    fmt::print("\n\n");

    ////////////////////////////////////////////////////////////////////////////
    // flop
    ////////////////////////////////////////////////////////////////////////////
    board.push_back(mkp::card{"Tc"});    // Ts9sTc
    const auto results_board_flop = calc_results(vec_hands, board);
    pretty_print(vec_hands, results_board_flop, board);
    fmt::print("\n\n");

    ////////////////////////////////////////////////////////////////////////////
    // turn
    ////////////////////////////////////////////////////////////////////////////
    board.push_back(mkp::card{"9c"});    // Ts9sTc9c
    const auto results_board_turn = calc_results(vec_hands, board);
    pretty_print(vec_hands, results_board_turn, board);
    fmt::print("\n\n");

    ////////////////////////////////////////////////////////////////////////////
    // river
    ////////////////////////////////////////////////////////////////////////////
    board.push_back(mkp::card{"Ah"});    // Ts9sTc9cAh
    const auto results_board_river = calc_results(vec_hands, board);
    pretty_print(vec_hands, results_board_river, board);
    fmt::print("\n\n");

    return EXIT_SUCCESS;
}