#pragma once

#include <mkpoker/base/card.hpp>
#include <mkpoker/base/cardset.hpp>

#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace mkpoker::base
{
    // templated container that holds two/four cards/ranks
    // for ordinary hands and 'ranges' (with rank as value_type)
    template <typename c_r_type, bool allow_duplicates, bool is_auto_ordered>
    class hand_helper
    {
        static_assert(std::is_same_v<typename c_r_type::mkpoker_char_count::value_type, uint8_t>,
                      "c_r_type must provide mkpoker_char_count as uint8_t");
        constexpr static auto char_count = c_r_type::mkpoker_char_count::value;

        // internal encoding
        c_r_type m_card1;
        c_r_type m_card2;

       public:
        ///////////////////////////////////////////////////////////////////////////////////////
        // CTORS
        ///////////////////////////////////////////////////////////////////////////////////////

        // we do not want invalid objects
        hand_helper() = delete;
        // create from two cards, may throw depending on template
        constexpr hand_helper(const c_r_type c1, const c_r_type c2) noexcept(allow_duplicates) : m_card1(c1), m_card2(c2)
        {
            if constexpr (!allow_duplicates)
            {
                if (c1 == c2)
                {
                    throw std::runtime_error("CTOR hand: tried to create hand with equal input (no duplicates allowed): " + c1.str() +
                                             c2.str());
                }
            }

            if constexpr (is_auto_ordered)
            {
                if (c1 > c2)
                {
                    m_card1 = c2;
                    m_card2 = c1;
                }
            }
        }
        // fast CTOR for convenience, may throw
        constexpr hand_helper(const int8_t i1, const int8_t i2) : hand_helper(c_r_type(i1), c_r_type(i2)) {}
        // create from string, may throw
        constexpr explicit hand_helper(const std::string_view str)
            : hand_helper(c_r_type{str.substr(0, char_count)}, c_r_type{str.substr(char_count, char_count)})
        {
            if (str.size() != 2 * char_count)
            {
                throw std::runtime_error(std::string("CTOR hand: tried to create a hand with a string of wrong size: '").append(str) + "'");
            }
        }
        // create from cardset, enable only for card
        template <typename T = c_r_type, std::enable_if_t<std::is_same_v<typename T::is_card_type, std::true_type>, int> = 0>
        constexpr explicit hand_helper(const cardset cs)
            : hand_helper(c_r_type{static_cast<int8_t>(cross_low_rank_64(cs.code_bit()))},
                          c_r_type{static_cast<int8_t>(cross_high_rank_64(cs.code_bit()))})
        {
            if (cs.size() != 2)
            {
                throw std::runtime_error(std::string("CTOR hand: tried to create a hand with a cardset of wrong size: '").append(cs.str()) +
                                         "'");
            }
        }

        //
        // all other constructors will be defaulted by the compiler

        ///////////////////////////////////////////////////////////////////////////////////////
        // public api
        ///////////////////////////////////////////////////////////////////////////////////////

        // return bit code
        [[nodiscard]] constexpr auto code_bit() const noexcept { return m_card1.code_bit() | m_card2.code_bit(); }

        // get both cards, enable only for card
        template <typename T = c_r_type, std::enable_if_t<std::is_same_v<typename T::is_card_type, std::true_type>, int> = 0>
        [[nodiscard]] constexpr auto get_cards() const noexcept
        {
            return std::make_tuple(m_card1, m_card2);
        }
        // get higher card, enable only for card
        template <typename T = c_r_type, std::enable_if_t<std::is_same_v<typename T::is_card_type, std::true_type>, int> = 0>
        [[nodiscard]] constexpr auto hi() const noexcept
        {
            return m_card1 < m_card2 ? m_card2 : m_card1;
        }
        // get lower card, enable only for card
        template <typename T = c_r_type, std::enable_if_t<std::is_same_v<typename T::is_card_type, std::true_type>, int> = 0>
        [[nodiscard]] constexpr auto lo() const noexcept
        {
            return m_card1 < m_card2 ? m_card1 : m_card2;
        }
        // get both ranks, enable only for rank
        template <typename T = c_r_type, std::enable_if_t<std::is_same_v<typename T::is_rank_type, std::true_type>, int> = 0>
        [[nodiscard]] constexpr auto get_ranks() const noexcept
        {
            return std::make_pair(m_card1, m_card2);
        }

        // return as cardset, enable only for cards
        template <typename T = c_r_type, std::enable_if_t<std::is_same_v<typename T::is_card_type, std::true_type>, int> = 0>
        [[nodiscard]] constexpr cardset to_cardset() const noexcept
        {
            return cardset(code_bit());
        }

        // return string representation, is noexcept since we only allow valid objects to be created
        [[nodiscard]] std::string str() const noexcept { return std::string(m_card1.str() + m_card2.str()); }

        //
        // operators

        constexpr bool operator<(const hand_helper& h) const noexcept
        {
            if (m_card1 == h.m_card1)
            {
                return m_card2 < h.m_card2;
            }
            return m_card1 < h.m_card1;
        }

    };

    // size: 16 bits
    //
    // a hand holds two cards, no duplicates allowed, automatically ordered by ascending value
    using hand = hand_helper<mkpoker::cards::card, false, true>;
    // size: 16 bits
    //
    // a hand_of_ranks holds two ranks, duplicates are allowed, not automatically ordered
    using hand_of_ranks = hand_helper<mkpoker::cards::rank, true, false>;

}    // namespace mkpoker::base