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

#include <array>
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
        enum class rank_t : uint8_t
        {
            two = 0,
            three,
            four,
            five,
            six,
            seven,
            eight,
            nine,
            ten,
            jack,
            queen,
            king,
            ace
        };

        constexpr uint8_t c_rank_two = 0;
        constexpr uint8_t c_rank_three = 1;
        constexpr uint8_t c_rank_four = 2;
        constexpr uint8_t c_rank_five = 3;
        constexpr uint8_t c_rank_six = 4;
        constexpr uint8_t c_rank_seven = 5;
        constexpr uint8_t c_rank_eight = 6;
        constexpr uint8_t c_rank_nine = 7;
        constexpr uint8_t c_rank_ten = 8;
        constexpr uint8_t c_rank_jack = 9;
        constexpr uint8_t c_rank_queen = 10;
        constexpr uint8_t c_rank_king = 11;
        constexpr uint8_t c_rank_ace = 12;
        constexpr uint8_t c_num_ranks = 13;

        constexpr uint8_t c_mask_ranks_numbers = 0b0000'1111;
        constexpr uint16_t c_mask_ranks = 0b0001'1111'1111'1111;

    }    // namespace constants

    // encodes ranks from two to ace as (0...12)
    class rank
    {
       public:
        //  encoding
        const uint8_t m_rank;

        ///////////////////////////////////////////////////////////////////////////////////////
        // CTORS
        ///////////////////////////////////////////////////////////////////////////////////////

        // we only allow valid ranks
        constexpr rank() = delete;

        // create from char, can throw
        constexpr explicit rank(const char c) : m_rank(from_char(c))
        {
            if (m_rank >= c_num_ranks)
            {
                throw std::runtime_error(std::string("rank(const char): could not parse char '") + c + "'");
            }
        }

        // create from 'number', can throw
        constexpr explicit rank(const rank_t rt) : m_rank(static_cast<uint8_t>(rt))
        {
            if (m_rank >= c_num_ranks)
            {
                throw std::runtime_error("rank(const rank_t): invalid number '" + std::to_string(m_rank) + "'");
            }
        }

        // create from string, can throw
        constexpr explicit rank(const std::string_view str) : m_rank(from_char(str[0]))
        {
            if (str.size() != 1)
            {
                throw std::runtime_error(std::string("rank(const string_view): string with wrong size '").append(str) + "'");
            }
            else if (m_rank >= c_num_ranks)
            {
                throw std::runtime_error(std::string("rank(const string_view): could not parse char '") + str[0] + "'");
            }
        }

        ///////////////////////////////////////////////////////////////////////////////////////
        // ACCESSORS
        ///////////////////////////////////////////////////////////////////////////////////////

        // return the rank bit set
        [[nodiscard]] constexpr uint16_t as_bitset() const noexcept { return uint16_t(1) << m_rank; }

        // return string representation
        [[nodiscard]] constexpr std::string str() const noexcept { return std::string(1, to_char(static_cast<rank_t>(m_rank))); }

        // nice printing
        [[nodiscard]] constexpr std::string_view str_nice_single() const noexcept
        {
            constexpr std::array str_representation{"Two",  "Three", "Four", "Five",  "Six",  "Seven", "Eight",
                                                    "Nine", "Ten",   "Jack", "Queen", "King", "Ace"};
            return str_representation[m_rank];
        }
        [[nodiscard]] constexpr std::string str_nice_mult() const noexcept
        {
            if (m_rank == c_rank_six)
            {
                return (std::string(str_nice_single()) + "es");
            }
            else
            {
                return (std::string(str_nice_single()) + "s");
            }
        }

        ///////////////////////////////////////////////////////////////////////////////////////
        // MUTATORS
        ///////////////////////////////////////////////////////////////////////////////////////

        // none

        ///////////////////////////////////////////////////////////////////////////////////////
        // helper functions
        ///////////////////////////////////////////////////////////////////////////////////////

        // we do provide ordering, so the type can be used with set, map etc.

        constexpr auto operator<=>(const rank&) const = default;

       private:
        // return matching char,
        // internal use only so the input value can always assumed to be valid
        [[nodiscard]] constexpr char to_char(const rank_t rt) const noexcept
        {
            switch (rt)
            {
                case rank_t::two:
                    return '2';
                case rank_t::three:
                    return '3';
                case rank_t::four:
                    return '4';
                case rank_t::five:
                    return '5';
                case rank_t::six:
                    return '6';
                case rank_t::seven:
                    return '7';
                case rank_t::eight:
                    return '8';
                case rank_t::nine:
                    return '9';
                case rank_t::ten:
                    return 'T';
                case rank_t::jack:
                    return 'J';
                case rank_t::queen:
                    return 'Q';
                case rank_t::king:
                    return 'K';
                case rank_t::ace:
                    return 'A';
                default:                   // LCOV_EXCL_LINE
                    mkp::unreachable();    // LCOV_EXCL_LINE
            }
        }

        // parse from char
        [[nodiscard]] constexpr uint8_t from_char(const char c) const noexcept
        {
            switch (c)
            {
                case '2':
                    return c_rank_two;
                case '3':
                    return c_rank_three;
                case '4':
                    return c_rank_four;
                case '5':
                    return c_rank_five;
                case '6':
                    return c_rank_six;
                case '7':
                    return c_rank_seven;
                case '8':
                    return c_rank_eight;
                case '9':
                    return c_rank_nine;
                case 't':
                case 'T':
                    return c_rank_ten;
                case 'j':
                case 'J':
                    return c_rank_jack;
                case 'q':
                case 'Q':
                    return c_rank_queen;
                case 'k':
                case 'K':
                    return c_rank_king;
                case 'a':
                case 'A':
                    return c_rank_ace;
                default:
                    return c_num_ranks;
            };
        }
    };

    static_assert(std::is_standard_layout_v<rank>, "rank should have standard layout");
    static_assert(std::has_unique_object_representations_v<rank>, "rank should have a unique representation / be hashable");

    static_assert(!std::is_default_constructible_v<rank>, "rank should NOT be default constructible");

    static_assert(std::is_trivially_copyable_v<rank>, "rank should be trivially & nothrow copy/move constructible");
    static_assert(std::is_trivially_copy_constructible_v<rank>, "rank should be trivially & nothrow copy/move constructible");
    static_assert(std::is_trivially_move_constructible_v<rank>, "rank should be trivially & nothrow copy/move constructible");
    static_assert(std::is_nothrow_copy_constructible_v<rank>, "rank should be trivially & nothrow copy/move constructible");
    static_assert(std::is_nothrow_move_constructible_v<rank>, "rank should be trivially & nothrow copy/move constructible");

}    // namespace mkp
