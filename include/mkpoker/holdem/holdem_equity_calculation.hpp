/*

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

#pragma once

#include <mkpoker/base/card.hpp>
#include <mkpoker/base/cardset.hpp>
#include <mkpoker/base/hand.hpp>
#include <mkpoker/holdem/holdem_evaluation.hpp>

#include <algorithm>    // std::max_element, std::count
#include <cstdint>
#include <numeric>      // std::reduce
#include <stdexcept>    // std::runtime_error

namespace mkp
{
    // return format for equitiy calculation, similar to http://www.propokertools.com/simulations
    struct equity_calculation_result_t
    {
        std::vector<uint32_t> m_wins;
        std::vector<uint32_t> m_ties;
        std::vector<float> m_equities;
    };

    // calculate equities for variable number of hands and board (optional)
    equity_calculation_result_t calculate_equities(const std::vector<hand_2c>& hands, const std::vector<card>& vec_board = {})
    {
        if (const auto sz = hands.size(); sz < 2 || sz > 9)
        {
            throw std::runtime_error(fmt::format("invalid number of hands (should be in the range of [2,9], given {})", sz));
        }

        const auto all_hole_cards = [&]() {
            cardset cs{};
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
            cardset cs{};
            for (auto&& card : vec_board)
            {
                cs.insert(card);
            }
            return cs;
        }();
        if (board.size() != vec_board.size())
        {
            throw std::runtime_error(fmt::format("board contains duplicate cards ({} cards given but only {} of them are unique)",
                                                 vec_board.size(), board.size()));
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
        auto calculate_and_store_results = [&](const cardset& additional_cards) {
            const auto runout = additional_cards.combine(board);
            std::vector<holdem_result> results;
            for (auto&& hand : hands)
            {
                results.emplace_back(evaluate_unsafe(runout.combine(hand.as_cardset())));
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
                        score[n] += static_cast<uint32_t>(hands.size());
                        break;
                    }
                }
            }
        };

        if (board.size() == 5)
        {
            // calculate immediately - we already have 5 cards
            calculate_and_store_results(cardset{});
        }
        else
        {
            for (uint8_t i = 0; i < c_deck_size; ++i)
            {
                card c1{i};
                if (all_fixed_cards.contains(c1))
                {
                    continue;
                }

                if (board.size() == 4)
                {
                    // calculate immediately - we already have 5 cards
                    const auto additional_cards = cardset{c1};
                    calculate_and_store_results(additional_cards);
                }
                else
                {
                    for (uint8_t j = i + 1; j < c_deck_size; ++j)
                    {
                        card c2{j};
                        if (all_fixed_cards.contains(c2))
                        {
                            continue;
                        }

                        if (board.size() == 3)
                        {
                            // calculate immediately - we already have 5 cards
                            const auto additional_cards = cardset{c1, c2};
                            calculate_and_store_results(additional_cards);
                        }
                        else
                        {
                            for (uint8_t k = j + 1; k < c_deck_size; ++k)
                            {
                                card c3{k};
                                if (all_fixed_cards.contains(c3))
                                {
                                    continue;
                                }

                                if (board.size() == 2)
                                {
                                    // calculate immediately - we already have 5 cards
                                    const auto additional_cards = cardset{c1, c2, c3};
                                    calculate_and_store_results(additional_cards);
                                }
                                else
                                {
                                    for (uint8_t l = k + 1; l < c_deck_size; ++l)
                                    {
                                        card c4{l};
                                        if (all_fixed_cards.contains(c4))
                                        {
                                            continue;
                                        }

                                        if (board.size() == 1)
                                        {
                                            // calculate immediately - we already have 5 cards
                                            const auto additional_cards = cardset{c1, c2, c3, c4};
                                            calculate_and_store_results(additional_cards);
                                        }
                                        else
                                        {
                                            // draw 5th card
                                            for (uint8_t m = l + 1; m < c_deck_size; ++m)
                                            {
                                                card c5{m};
                                                if (all_fixed_cards.contains(c5))
                                                {
                                                    continue;
                                                }
                                                // calc and store results
                                                const auto additional_cards = cardset{c1, c2, c3, c4, c5};
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

        return equity_calculation_result_t{wins, ties, equities};
    }

}    // namespace mkp