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

#include <cstdint>
#include <iterator>
#include <numeric>
#include <vector>

namespace mkp
{
    std::vector<float> normalize(const std::vector<int32_t>& v)
    {
        int64_t sum = std::reduce(v.cbegin(), v.cend());
        if (sum > 0)
        {
            std::vector<float> ret;
            ret.reserve(v.size());
            std::transform(v.cbegin(), v.cend(), std::back_inserter(ret),
                           [sum](const int32_t i) -> float { return static_cast<float>(i) / sum; });
            return ret;
        }
        else
        {
            return std::vector<float>(v.size(), 1.0f / v.size());
        }
    }
}    // namespace mkp
