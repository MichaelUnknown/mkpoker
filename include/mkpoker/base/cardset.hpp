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
#include <mkpoker/util/bit.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <initializer_list>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace mkp
{
    inline namespace constants
    {
        constexpr uint64_t c_cardset_full = 0xFFFF'FFFF'FFFF'FFFF >> (64 - c_deck_size);
    }    // namespace constants

    // encodes the first 52 bits as cards in canonical order (ascending clubs,diamonds,hearts,spades)
    class cardset
    {
        // internal encoding
        uint64_t m_cards = 0;

        // private helper to create from bitset (needed for rotate suits and combine)
        constexpr auto set(const uint64_t& bitset) noexcept
        {
            m_cards = bitset;
            return *this;
        }

       public:
        ///////////////////////////////////////////////////////////////////////////////////////
        // CTORS
        ///////////////////////////////////////////////////////////////////////////////////////

        // empty cardset
        cardset() = default;

        // create from bitset
        constexpr explicit cardset(const uint64_t& bitset) : m_cards(bitset)
        {
            if (bitset > c_cardset_full)
            {
                throw std::runtime_error("cardset(const uint64_t): cardset max value (" + std::to_string(c_cardset_full) +
                                         ") exceeded by argument " + std::to_string(bitset));
            }
        }

        // create with span (works for any contiguous container)
        constexpr explicit cardset(const std::span<const card> sp) noexcept
        {
            for (auto&& c : sp)
            {
                m_cards |= c.as_bitset();
            }
        }

        // create with init list
        constexpr explicit cardset(const std::initializer_list<const card> li) noexcept
        {
            for (auto&& c : li)
            {
                m_cards |= c.as_bitset();
            }
        }

        // create from string, i.e. "AcKs2h", throws if ill-formed
        constexpr explicit cardset(const std::string_view sv)
        {
            if (0 != sv.size() % 2)
            {
                throw std::runtime_error(std::string("string with wrong size: '").append(sv) + "'");
            }
            else
            {
                for (size_t i = 0; i < sv.size(); i += 2)
                {
                    m_cards |= card{sv.substr(i, 2)}.as_bitset();
                }
            }
        }

        ///////////////////////////////////////////////////////////////////////////////////////
        // ACCESSORS
        ///////////////////////////////////////////////////////////////////////////////////////

        // return size / number of unique cards
        [[nodiscard]] constexpr size_t size() const noexcept { return std::popcount(m_cards); }

        // return bit mask
        [[nodiscard]] constexpr uint64_t as_bitset() const noexcept { return m_cards; }

        // return string representation
        [[nodiscard]] std::string str() const noexcept
        {
            std::string out{};
            for (uint64_t mask = m_cards; mask;)
            {
                // create a card by getting the position of first bit
                const auto idx = cross_idx_low64(mask);
                out += card(idx).str();
                // clear that bit
                mask &= mask - 1;
            }
            return out;
        }

        // check if the card is in the set
        [[nodiscard]] constexpr bool contains(const card c) const noexcept { return (m_cards & uint64_t(1) << c.m_card) != 0; }

        // check if all cards from cs are in the set (i.e. cs is a subset)
        [[nodiscard]] constexpr bool contains(const cardset& cs) const noexcept { return (m_cards | cs.m_cards) == m_cards; }

        // check if cs is exclusive to this
        [[nodiscard]] constexpr bool disjoint(const cardset& cs) const noexcept { return (m_cards & cs.m_cards) == 0; }

        // check if cs hs joined cards to this
        [[nodiscard]] constexpr bool intersects(const cardset& cs) const noexcept { return (m_cards & cs.m_cards) != 0; }

        // returns a new cs, combine with card
        [[nodiscard]] constexpr cardset combine(const card c) const noexcept { return cardset{}.set(m_cards | uint64_t(1) << c.m_card); }

        // returns a new cs, combine with cardset
        [[nodiscard]] constexpr cardset combine(const cardset& cs) const noexcept { return cardset{}.set(m_cards | cs.m_cards); }

        // get the suit rotation vector that transforms this cardset into the normalized form
        [[nodiscard]] constexpr std::array<uint8_t, 4> get_normalization_vector() const noexcept
        {
            // break down the cards into individual suits and sort by amount of cards, then highest card
            // then return the vector which performs this exact transformation
            const uint16_t mask_c = m_cards & c_mask_ranks;
            const uint16_t mask_d = (m_cards >> (1 * c_num_ranks)) & c_mask_ranks;
            const uint16_t mask_h = (m_cards >> (2 * c_num_ranks)) & c_mask_ranks;
            const uint16_t mask_s = (m_cards >> (3 * c_num_ranks)) & c_mask_ranks;

            using pui16 = std::pair<uint16_t, uint16_t>;
            std::array<pui16, 4> temp{{{uint16_t(0), mask_c}, {uint16_t(1), mask_d}, {uint16_t(2), mask_h}, {uint16_t(3), mask_s}}};

            std::sort(temp.begin(), temp.end(), [](const pui16& lhs, const pui16& rhs) {
                if (std::popcount(lhs.second) == std::popcount(rhs.second))
                {
                    return lhs.second > rhs.second;
                }
                return std::popcount(lhs.second) > std::popcount(rhs.second);
            });

            std::array<uint8_t, 4> ret{};
            for (uint8_t u = 0; u < 4; ++u)
            {
                ret[temp[u].first] = u;
            }
            return ret;
        }

        // returns a new cs, rotate the suits
        [[nodiscard]] constexpr cardset rotate_suits(const std::array<uint8_t, 4>& r) const
        {
            if (const int i = (1 << r[0] | 1 << r[1] | 1 << r[2] | 1 << r[3]); i != 0b0'1111)
            {
                throw std::runtime_error("duplicate or invalid arguments provided for rotate_suits");
            }

            return cardset{}.set((m_cards & c_mask_ranks) << c_num_ranks * r[0] |
                                 ((m_cards >> (1 * c_num_ranks)) & c_mask_ranks) << c_num_ranks * r[1] |
                                 ((m_cards >> (2 * c_num_ranks)) & c_mask_ranks) << c_num_ranks * r[2] |
                                 ((m_cards >> (3 * c_num_ranks)) & c_mask_ranks) << c_num_ranks * r[3]);
        }

        // returns as vector of cards
        [[nodiscard]] std::vector<card> as_cards() const noexcept
        {
            std::vector<card> ret{};
            ret.reserve(size());

            for (uint64_t mask = m_cards; mask;)
            {
                // create a card by getting the position of lowest bit
                const auto idx = cross_idx_low64(mask);
                ret.emplace_back(card(idx));
                // clear that bit
                mask &= mask - 1;
            }
            return ret;
        }

        ///////////////////////////////////////////////////////////////////////////////////////
        // MUTATORS
        ///////////////////////////////////////////////////////////////////////////////////////

        // clear the set
        constexpr void clear() noexcept { m_cards = 0; }

        // fill with all cards
        constexpr void fill() noexcept { m_cards = c_cardset_full; }

        // insert a single card
        constexpr void insert(const card c) noexcept { m_cards |= c.as_bitset(); }

        // join with another set
        constexpr void join(const cardset cs) noexcept { m_cards |= cs.m_cards; }

        // remove a single card
        constexpr void remove(const card c) noexcept { m_cards ^= c.as_bitset(); }

        // remove all cards from cs (if they exist)
        constexpr void remove(const cardset cs) noexcept { m_cards ^= cs.m_cards; }

        ///////////////////////////////////////////////////////////////////////////////////////
        // helper functions
        ///////////////////////////////////////////////////////////////////////////////////////

        constexpr auto operator<=>(const cardset&) const noexcept = default;
    };

}    // namespace mkp
