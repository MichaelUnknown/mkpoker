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

#include <bit>
#include <cstdint>

namespace mkp
{
    // make a bitset out of ints
    template <typename... Args>
    [[nodiscard]] constexpr uint64_t make_bitset(Args... args)
    {
        static_assert((std::is_integral_v<Args> && ...), "make_bitset(Args...) expects integral types");
        return ((uint64_t(1) << args) | ...);
    }

    // wrappers

    constexpr uint8_t cross_idx_low16(const uint16_t mask)
    {
        if (mask == 0)
            return 0;
        return static_cast<uint8_t>(std::countr_zero(mask));
    }
    constexpr uint8_t cross_idx_low64(const uint64_t mask)
    {
        if (mask == 0)
            return 0;
        return static_cast<uint8_t>(std::countr_zero(mask));
    }
    constexpr uint8_t cross_idx_high16(const uint16_t mask)
    {
        if (mask == 0)
            return 0;
        return static_cast<uint8_t>(15 - std::countl_zero(mask));
    }
    constexpr uint8_t cross_idx_high64(const uint64_t mask)
    {
        if (mask == 0)
            return 0;
        return static_cast<uint8_t>(63 - std::countl_zero(mask));
    }
}    // namespace mkp
