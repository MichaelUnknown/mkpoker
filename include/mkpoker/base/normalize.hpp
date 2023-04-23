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
#include <mkpoker/base/hand.hpp>

#include <array>
#include <cstdint>
#include <stdexcept>

namespace mkp
{
    auto suit_normalization_permutation(const hand_2c h, const cardset& b) -> std::array<uint8_t, 4>
    {
        const auto board_size = b.size();
        const auto cs_all = b.combine(h.as_cardset());

        if (board_size < 3 || board_size > 5)
        {
            throw std::runtime_error("suit_normalization_permutation: called with invalid board size");
        }
        if (cs_all.size() != (board_size + 2))
        {
            throw std::runtime_error("suit_normalization_permutation: called with duplicated cards");
        }

        // clang-format off
        static constexpr std::array<std::array<std::array<uint8_t, 4>, 4>, 4> choose {{
            {{ {0,1,2,3}, {0,1,2,3}, {0,2,1,3}, {0,3,1,2} }},
            {{ {1,0,2,3}, {1,0,2,3}, {1,2,0,3}, {1,3,0,2} }},
            {{ {2,0,1,3}, {2,1,0,3}, {2,0,1,3}, {2,3,0,1} }},
            {{ {3,0,1,2}, {3,1,0,2}, {3,2,0,1}, {3,0,1,2} }}
        }};

        // clang-format on
        std::array<uint8_t, 4> temp = choose[h.m_card1.suit().m_suit][h.m_card2.suit().m_suit];

        // special case: suited hand
        if (h.m_card1.suit() == h.m_card2.suit())
        {
            using namespace ::mkp::constants;

            const uint16_t mask_1 = (cs_all.as_bitset() >> (temp[1] * c_num_ranks)) & c_mask_ranks;
            const uint16_t mask_2 = (cs_all.as_bitset() >> (temp[2] * c_num_ranks)) & c_mask_ranks;
            const uint16_t mask_3 = (cs_all.as_bitset() >> (temp[3] * c_num_ranks)) & c_mask_ranks;

            using pui16 = std::pair<uint8_t, uint16_t>;
            std::array<pui16, 4> tnew{{{temp[0], uint16_t(0)}, {temp[1], mask_1}, {temp[2], mask_2}, {temp[3], mask_3}}};

            std::sort(tnew.begin() + 1, tnew.end(), [](const pui16& lhs, const pui16& rhs) {
                if (std::popcount(lhs.second) == std::popcount(rhs.second))
                {
                    return lhs.second > rhs.second;
                }
                return std::popcount(lhs.second) > std::popcount(rhs.second);
            });

            temp[1] = tnew[1].first;
            temp[2] = tnew[2].first;
            temp[3] = tnew[3].first;
        }
        else
        {
            using namespace ::mkp::constants;

            if (h.m_card1.rank() == h.m_card2.rank())
            {
                const uint16_t mask_1 = (cs_all.as_bitset() >> (temp[0] * c_num_ranks)) & c_mask_ranks;
                const uint16_t mask_2 = (cs_all.as_bitset() >> (temp[1] * c_num_ranks)) & c_mask_ranks;
                if (mask_1 < mask_2)
                {
                    std::swap(temp[0], temp[1]);
                }
            }
            const uint16_t mask_3 = (cs_all.as_bitset() >> (temp[2] * c_num_ranks)) & c_mask_ranks;
            const uint16_t mask_4 = (cs_all.as_bitset() >> (temp[3] * c_num_ranks)) & c_mask_ranks;
            if (mask_3 < mask_4)
            {
                std::swap(temp[2], temp[3]);
            }
            // else positions 3 and 4 are in correct order from the lookup table
        }

        // "invert" numbers so we return the wanted permutation to get that conanical suits
        std::array<uint8_t, 4> ret{};
        for (uint8_t u = 0; u < 4; ++u)
        {
            ret[temp[u]] = u;
        }

        return ret;
    }

}    // namespace mkp
