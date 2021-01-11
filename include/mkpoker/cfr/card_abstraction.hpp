/*

Copyright (C) 2020 Michael Knörzer

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

#include <mkpoker/base/range.hpp>
#include <mkpoker/base/rank.hpp>
#include <mkpoker/game/game.hpp>
#include <mkpoker/util/mtp.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace mkp
{
    template <std::size_t N, UnsignedIntegral T = uint32_t>
    struct card_abstraction_base
    {
        using uint_type = T;

        virtual ~card_abstraction_base() = default;

        [[nodiscard]] virtual uint_type size(const gb_gamestate_t game_state) const = 0;
        [[nodiscard]] virtual uint_type id(const gb_gamestate_t game_state, const uint8_t active_player,
                                           const gamecards<N>& cards) const = 0;

        // debug / human readable description of that abstraction
        [[nodiscard]] virtual std::string str_id(const gb_gamestate_t game_state, uint_type id) const = 0;
    };

    // sample card abstraction that buckets cards by range classification, i.e. AA, AKs, AKo etc.
    // keep in mind, that this abstraction only works preflop
    template <std::size_t N, UnsignedIntegral T = uint32_t>
    struct card_abstraction_by_range final : public card_abstraction_base<N, T>
    {
        using typename card_abstraction_base<N, T>::uint_type;

        [[nodiscard]] virtual uint_type size([[maybe_unused]] const gb_gamestate_t game_state) const override
        {
            return c_num_ranks * c_num_ranks;
        }

        [[nodiscard]] virtual uint_type id([[maybe_unused]] const gb_gamestate_t game_state, const uint8_t active_player,
                                           const gamecards<N>& cards) const override
        {
            return range::index(cards.m_hands[active_player]);
        }

        // debug / human readable description of that id
        [[nodiscard]] virtual std::string str_id([[maybe_unused]] const gb_gamestate_t game_state, uint_type id) const override
        {
            return range::hand(static_cast<uint8_t>(id)).str() + " (" + std::to_string(id) + ")";
        }
    };

}    // namespace mkp
