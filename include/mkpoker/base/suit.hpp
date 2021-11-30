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

#include <mkpoker/util/utility.hpp>

#include <compare>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>

namespace mkp
{
    inline namespace constants
    {
        enum class suit_t : uint8_t
        {
            clubs = 0,
            diamonds,
            hearts,
            spades
        };

        constexpr uint8_t c_suit_clubs = 0;
        constexpr uint8_t c_suit_diamonds = 1;
        constexpr uint8_t c_suit_hearts = 2;
        constexpr uint8_t c_suit_spades = 3;
        constexpr uint8_t c_num_suits = 4;
    }    // namespace constants

    // encodes suit in alphabetical order (clubs,diamonds,hearts,spades) as {0,1,2,3}
    class suit
    {
       public:
        // encoding
        const uint8_t m_suit;

        ///////////////////////////////////////////////////////////////////////////////////////
        // CTORS
        ///////////////////////////////////////////////////////////////////////////////////////

        // we only allow valid suits
        constexpr suit() = delete;

        // create from char (acdh), can throw
        constexpr explicit suit(const char c) : m_suit(from_char(c))
        {
            if (m_suit >= c_num_suits)
            {
                throw std::runtime_error(std::string("suit(const char): could not parse char '") + c + "'");
            }
        }

        // create from 'number', can throw
        constexpr explicit suit(const suit_t st) : m_suit(static_cast<uint8_t>(st))
        {
            if (m_suit >= c_num_suits)
            {
                throw std::runtime_error("suit(const suit_t): invalid number '" + std::to_string(m_suit) + "'");
            }
        }

        // create from string, can throw
        constexpr explicit suit(const std::string_view str) : m_suit(from_char(str[0]))
        {
            if (str.size() != 1)
            {
                throw std::runtime_error(std::string("suit(const string_view): string with wrong size '").append(str) + "'");
            }
            else if (m_suit >= c_num_suits)
            {
                throw std::runtime_error(std::string("suit(const string_view): could not parse char '") + str[0] + "'");
            }
        }

        ///////////////////////////////////////////////////////////////////////////////////////
        // ACCESSORS
        ///////////////////////////////////////////////////////////////////////////////////////

        // return string representation
        [[nodiscard]] std::string str() const noexcept { return std::string(1, to_char(static_cast<suit_t>(m_suit))); }

        ///////////////////////////////////////////////////////////////////////////////////////
        // MUTATORS
        ///////////////////////////////////////////////////////////////////////////////////////

        // none

        ///////////////////////////////////////////////////////////////////////////////////////
        // helper functions
        ///////////////////////////////////////////////////////////////////////////////////////

        // we do provide ordering, so the type can be used with set, map etc.

        constexpr auto operator<=>(const suit&) const noexcept = default;

       private:
        // return matching char,
        // internal use only so the input value can always assumed to be valid
        [[nodiscard]] constexpr char to_char(const suit_t st) const noexcept
        {
            switch (st)
            {
                    // TODO: use 'using enum suit_t;' when supported by all compilers
                    // and intellisense

                case suit_t::clubs:
                    return 'c';
                case suit_t::diamonds:
                    return 'd';
                case suit_t::hearts:
                    return 'h';
                case suit_t::spades:
                    return 's';
                default:                   // GCOV_EXCL_LINE
                    mkp::unreachable();    // GCOV_EXCL_LINE
            }
        }

        // parse from char
        [[nodiscard]] constexpr uint8_t from_char(const char c) const noexcept
        {
            switch (c)
            {
                case 'c':
                case 'C':
                    return c_suit_clubs;
                case 'd':
                case 'D':
                    return c_suit_diamonds;
                case 'h':
                case 'H':
                    return c_suit_hearts;
                case 's':
                case 'S':
                    return c_suit_spades;
                default:
                    return c_num_suits;
            };
        }
    };

    static_assert(std::is_standard_layout_v<suit>, "suit should have standard layout");
    static_assert(std::has_unique_object_representations_v<suit>, "suit should have a unique representation / be hashable");

    static_assert(!std::is_default_constructible_v<suit>, "suit should NOT be default constructible");

    static_assert(std::is_trivially_copyable_v<suit>, "suit should be trivially & nothrow copy/move constructible");
    static_assert(std::is_trivially_copy_constructible_v<suit>, "suit should be trivially & nothrow copy/move constructible");
    static_assert(std::is_trivially_move_constructible_v<suit>, "suit should be trivially & nothrow copy/move constructible");
    static_assert(std::is_nothrow_copy_constructible_v<suit>, "suit should be trivially & nothrow copy/move constructible");
    static_assert(std::is_nothrow_move_constructible_v<suit>, "suit should be trivially & nothrow copy/move constructible");

}    // namespace mkp
