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

#include <mkpoker/game/game.hpp>
#include <mkpoker/util/mtp.hpp>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace mkp
{
    template <typename T, UnsignedIntegral U = uint32_t>
    struct game_abstraction_base
    {
        using game_type = T;
        using uint_type = U;

        virtual ~game_abstraction_base() = default;

        // takes a new gamestate as input and returns the id
        virtual uint_type encode(const game_type& gamestate) = 0;

        // converts the id back to the actual gamestate
        [[nodiscard]] virtual game_type decode(const uint_type id) const = 0;
    };

    // sample encoder that stores / enumerates the gamestates
    template <typename T, UnsignedIntegral U = uint32_t>
    struct gamestate_enumerator final : public game_abstraction_base<T, U>
    {
        using typename game_abstraction_base<T, U>::game_type;
        using typename game_abstraction_base<T, U>::uint_type;

        uint_type index = 0;
        std::vector<game_type> storage = {};

        virtual uint_type encode(const game_type& gamestate) override
        {
            storage.push_back(gamestate);
            return index++;
        }

        [[nodiscard]] virtual game_type decode(const uint_type id) const override { return storage.at(id); }
    };

}    // namespace mkp
