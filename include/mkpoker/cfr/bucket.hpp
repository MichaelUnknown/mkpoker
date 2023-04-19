/*

Copyright (C) Michael Kn�rzer

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

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <fmt/core.h>

namespace mkp
{
    struct hand_board_t
    {
        mkp::cardset h, b;
        [[nodiscard]] std::string str() const noexcept { return fmt::format("{}/{}", h.str(), b.str()); }
        constexpr auto operator<=>(const hand_board_t&) const noexcept = default;

        // serialize
        template <typename Archive>
        void serialize(Archive& ar, const unsigned int version)
        {
            boost::serialization::split_member(ar, *this, version);
        }
        template <typename Archive>
        void save(Archive& ar, const unsigned int version) const
        {
            ar << h.as_bitset();
            ar << b.as_bitset();
        }
        template <typename Archive>
        void load(Archive& ar, const unsigned int version)
        {
            uint64_t tmp;
            ar >> tmp;
            h = mkp::cardset(tmp);
            ar >> tmp;
            b = mkp::cardset(tmp);
        }
    };
}    // namespace mkp
