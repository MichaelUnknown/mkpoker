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

struct result_t
{
    std::vector<uint32_t> m_wins;
    std::vector<uint32_t> m_ties;
    std::vector<float> m_equities;
};

void pretty_print(const std::vector<mkp::hand_2c>& hands, const result_t& results, const std::vector<mkp::card>& vec_board = {})
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
    fmt::print(" Hand | Equity |      Wins |      Ties \n");
    fmt::print("------+--------+-----------+-----------\n");
    for (unsigned i = 0; i < hands.size(); ++i)
    {
        fmt::print(" {} | {:05.2f}% | {:>9} | {:>9}\n", hands[i].str(), results.m_equities[i], results.m_wins[i], results.m_ties[i]);
    }
}

result_t calc_results(const std::vector<mkp::hand_2c>& hands, const std::vector<mkp::card>& vec_board = {})
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
    if (const auto total = board.combine(all_hole_cards).size(); total != board.size() + all_hole_cards.size())
    {
        throw std::runtime_error(
            fmt::format("hands and board contain duplicate cards: number of unique cards ({}) is smaller than {} ({}[number of board "
                        "cards] + {}[number of cards in all hands])",
                        total, board.size() + all_hole_cards.size(), board.size(), all_hole_cards.size()));
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

    // calc and store results
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
                }
            }
        }
    };

    for (uint8_t i = 0; i < c_deck_size; ++i)
    {
        mkp::card c1{i};
        if (all_hole_cards.contains(c1))
        {
            continue;
        }
        for (uint8_t j = i + 1; j < c_deck_size; ++j)
        {
            mkp::card c2{j};
            if (all_hole_cards.contains(c2))
            {
                continue;
            }
            for (uint8_t k = j + 1; k < c_deck_size; ++k)
            {
                mkp::card c3{k};
                if (all_hole_cards.contains(c3))
                {
                    continue;
                }
                for (uint8_t l = k + 1; l < c_deck_size; ++l)
                {
                    mkp::card c4{l};
                    if (all_hole_cards.contains(c4))
                    {
                        continue;
                    }
                    for (uint8_t m = l + 1; m < c_deck_size; ++m)
                    {
                        mkp::card c5{m};
                        if (all_hole_cards.contains(c5))
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

    // calc actual equity and return
    const auto total_score = std::reduce(score.cbegin(), score.cend(), uint32_t(0));
    for (unsigned i = 0; i < hands.size(); ++i)
    {
        equities[i] = static_cast<float>(score[i]) / total_score;
        equities[i] *= 100;
    }

    return result_t{wins, ties, equities};
}

int main()
{
    ////////////////////////////////////////////////////////////////////////////
    // preflop calculation, no board
    ////////////////////////////////////////////////////////////////////////////
    mkp::hand_2c pf_h1{"AcAd"};
    mkp::hand_2c pf_h2{"Th9h"};
    std::vector<mkp::hand_2c> vec_hands_pf{pf_h1, pf_h2};

    const auto results_pf = calc_results(vec_hands_pf);
    pretty_print(vec_hands_pf, results_pf);
    fmt::print("\n");

    // one board card, just to test the code
    std::vector<mkp::card> board_test{mkp::card{"Ts"}};
    const auto results_board_test = calc_results(vec_hands_pf, board_test);
    pretty_print(vec_hands_pf, results_board_test, board_test);

    return EXIT_SUCCESS;
}