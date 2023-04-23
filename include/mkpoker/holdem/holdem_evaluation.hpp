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

#include <mkpoker/base/cardset.hpp>
#include <mkpoker/holdem/holdem_lookup_tables.hpp>
#include <mkpoker/holdem/holdem_result.hpp>
#include <mkpoker/util/bit.hpp>

#include <cstdint>
#include <stdexcept>

namespace mkp
{
    // this algorithm assumes that the cardset contains at most 7 cards; otherwise, the behavior is undefined
    [[nodiscard]] auto evaluate_unsafe(const cardset cs) noexcept
    {
        // general idea:
        // check the different hand types from highest to lowest, i.e. straight flush -> quads -> full house -> etc.

        // break down into suits
        const uint64_t mask = cs.as_bitset();
        const uint16_t mask_c = (mask >> (0 * c_num_ranks)) & c_mask_ranks;
        const uint16_t mask_d = (mask >> (1 * c_num_ranks)) & c_mask_ranks;
        const uint16_t mask_h = (mask >> (2 * c_num_ranks)) & c_mask_ranks;
        const uint16_t mask_s = (mask >> (3 * c_num_ranks)) & c_mask_ranks;

        // 1)
        // check flush first, if we found one, there can be no quads / fh
        // so we return (straight) flush as a result
        {
            // if we have a straight, return straight flush high, else return flush
            auto flush_or_straight_flush = [](const uint16_t mask_flush) {
                const auto x = mkpoker_table_straight[mask_flush];
                if (x > 0)
                {
                    return holdem_result(c_straight_flush, x, 0, 0);
                }
                else
                {
                    return holdem_result(c_flush, 0, 0, mkpoker_table_top5[mask_flush]);
                }
            };

            if (std::popcount(mask_c) >= 5)
            {
                return flush_or_straight_flush(mask_c);
            }
            else if (std::popcount(mask_d) >= 5)
            {
                return flush_or_straight_flush(mask_d);
            }
            else if (std::popcount(mask_h) >= 5)
            {
                return flush_or_straight_flush(mask_h);
            }
            else if (std::popcount(mask_s) >= 5)
            {
                return flush_or_straight_flush(mask_s);
            }
        }

        // 2)
        // check for quads, full house

        // this mask is used for quads, fh and pairs
        const uint16_t mask_all_cards = mask_c | mask_d | mask_h | mask_s;

        // check quads
        if (const uint16_t mask_quads = (mask_c & mask_d & mask_h & mask_s); mask_quads)
        {
            return holdem_result(c_four_of_a_kind, cross_idx_high16(mask_quads), 0,
                                 (uint16_t(1) << cross_idx_high16(mask_all_cards & ~mask_quads)));
        }

        // this mask is used for for full house and trips
        const uint16_t mask_trips = ((mask_c & mask_d) | (mask_h & mask_s)) & ((mask_c & mask_h) | (mask_d & mask_s));

        // check for full house (trips + pair)
        if (mask_trips)
        {
            // mask_pair_fh below checks for exactly 2 identical cards and can thus miss if we have trips + trips
            // since we only evaluate 7 cards at max, there can be no other pairs in case of double trips
            if (std::popcount(mask_trips) > 1)
            {
                return holdem_result(c_full_house, cross_idx_high16(mask_trips), cross_idx_low16(mask_trips), 0);
            }

            // this finds all the duplicated cards, but no trips/quads
            if (const uint16_t mask_pair_fh = (mask_all_cards ^ (mask_c ^ mask_d ^ mask_h ^ mask_s)); mask_pair_fh)
            {
                return holdem_result(c_full_house, cross_idx_high16(mask_trips), cross_idx_high16(mask_pair_fh), 0);
            }
        }

        // 3)
        // check for straight
        const auto rank_straight = mkpoker_table_straight[mask_all_cards];
        if (rank_straight > 0)
        {
            return holdem_result(c_straight, rank_straight, 0, 0);
        }

        // 4)
        // check trips
        if (mask_trips)
        {
            const uint16_t mask_kickers = mask_all_cards & ~(mask_trips);
            const auto high_kicker = cross_idx_high16(mask_kickers);
            const auto low_kicker = cross_idx_high16(mask_kickers & ~(uint16_t(1) << high_kicker));
            return holdem_result(c_three_of_a_kind, cross_idx_high16(mask_trips), 0,
                                 uint16_t(1) << high_kicker | uint16_t(1) << low_kicker);
        }

        // 5)
        // pair / two pair
        const uint16_t mask_pair = (mask_all_cards ^ (mask_c ^ mask_d ^ mask_h ^ mask_s));
        if (const auto num_pairs = std::popcount(mask_pair); num_pairs > 1)
        {
            // get the two highest ranks from the mask (keep in mind - with 6/7 cards, there can be 3 pairs)
            const auto high_rank = cross_idx_high16(mask_pair);
            const auto low_rank = cross_idx_high16(mask_pair & ~(uint16_t(1) << high_rank));
            // from the remaining cards, get the highest rank
            const auto kicker_rank = cross_idx_high16(mask_all_cards & ~(uint16_t(1) << high_rank | uint16_t(1) << low_rank));
            return holdem_result(c_two_pair, high_rank, low_rank, uint16_t(1) << kicker_rank);
        }
        else if (num_pairs > 0)
        {
            const uint16_t mask_kickers = mask_all_cards & ~(mask_pair);
            return holdem_result(c_one_pair, cross_idx_high16(mask_pair), 0, mkpoker_table_top3[mask_kickers]);
        }

        // 6)
        // no pair
        return holdem_result(c_no_pair, 0, 0, mkpoker_table_top5[mask_all_cards]);
    }

    // safe call, will check if the cardset provides a suitable cardset
    [[nodiscard]] auto evaluate_safe(const cardset cs)
    {
        if (const auto size = cs.size(); size > 7 || size < 5)
        {
            throw std::runtime_error("evaluate_safe() called with cardset of wrong size: " + std::to_string(size));
        }
        return evaluate_unsafe(cs);
    }

    // variadic overload
    template <typename T, typename... TArgs>
    [[nodiscard]] auto evaluate_safe(const cardset cs, const T value, const TArgs... args)
    {
        return evaluate_safe(cs.combine(value), args...);
    }

}    // namespace mkp