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

#include <array>
#include <cstddef>

namespace mkp
{
    template <typename T, std::size_t N>
    constexpr std::array<T, N>& operator+=(std::array<T, N>& lhs, const std::array<T, N>& rhs)
    {
        for (std::size_t i = 0; i < N; ++i)
            lhs[i] += rhs[i];
        return lhs;
    }

    template <typename T, std::size_t N>
    constexpr std::array<T, N> operator+(const std::array<T, N>& lhs, const std::array<T, N>& rhs)
    {
        std::array<T, N> result = lhs;
        result += rhs;
        return result;
    }
}    // namespace mkp
