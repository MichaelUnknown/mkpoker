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

#include <cstdint>
#include <random>
#include <stdexcept>
#include <vector>

namespace mkp
{
    template <uint8_t max_rank = c_rank_ace, uint8_t max_suit = c_suit_spades>
    class card_generator
    {
        static_assert(max_rank >= c_rank_two && max_rank <= c_rank_ace, "max_rank value out of range");
        static_assert(max_suit >= c_suit_clubs && max_suit <= c_suit_spades, "max_suit value out of range");

        std::mt19937 rng;
        std::uniform_int_distribution<> dist_rank;
        std::uniform_int_distribution<> dist_suit;

       public:
        constexpr card_generator(const unsigned int seed = 1927) : rng(seed), dist_rank(0, max_rank), dist_suit(0, max_suit) {}

        [[nodiscard]] card generate() { return card{rank{rank_t{uint8_t(dist_rank(rng))}}, suit{suit_t{uint8_t(dist_suit(rng))}}}; }

        [[nodiscard]] std::vector<card> generate_v(uint8_t n)
        {
            if (n > ((max_rank + 1) * (max_suit + 1)))
            {
                throw std::runtime_error("generate_v: n greater than number of specified unique cards");
            }

            std::vector<uint8_t> cards;
            cards.reserve((max_rank + 1) * (max_suit + 1));
            for (uint8_t i = 0; i <= max_rank; ++i)
            {
                for (uint8_t j = 0; j <= max_suit; ++j)
                {
                    cards.emplace_back(static_cast<uint8_t>(i + j * c_num_ranks));
                }
            }
            std::shuffle(cards.begin(), cards.end(), rng);
            cards.resize(n);

            std::vector<card> ret;
            ret.reserve(n);
            //std::for_each(cards.cbegin(), cards.cend(), [&](const uint8_t i) { ret.emplace_back(i); });
            for (auto&& i : cards)
            {
                ret.emplace_back(i);
            }
            return ret;
        }
    };

}    // namespace mkp