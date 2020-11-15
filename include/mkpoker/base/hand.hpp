#pragma once

#include <mkpoker/base/card.hpp>
#include <mkpoker/base/cardset.hpp>

#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
//#include <utility>

namespace mkpoker::base
{
    inline namespace old
    {
        //
        // some TMP helpers

        template <typename T>
        using is_card = std::is_same<T, typename mkpoker::base::card>;
        template <typename T>
        constexpr bool is_card_v = is_card<T>::value;
        template <typename T>
        using is_rank = std::is_same<T, typename mkpoker::base::rank>;
        template <typename T>
        constexpr bool is_rank_v = is_rank<T>::value;
        template <typename T>
        using is_card_or_rank = std::disjunction<is_card<T>, is_rank<T>>;
        template <typename T>
        constexpr bool is_card_or_rank_v = is_card_or_rank<T>::value;

        // helper class for hand_2c, hand_2r
        template <typename T, bool allow_duplicates, bool is_auto_ordered, std::enable_if_t<is_card_or_rank_v<T>, int> = 0>
        class hand_helper
        {
            using c_r_type = T;
            constexpr static auto char_count = is_card_v<c_r_type> ? 2 : 1;

            // internal encoding

            c_r_type m_card1;
            c_r_type m_card2;

            //
            // private helper functions

            [[nodiscard]] constexpr auto choose_1(const c_r_type c1, const c_r_type c2) const noexcept
            {
                if constexpr (is_auto_ordered)
                {
                    if (c1 < c2)
                    {
                        return c1;
                    }
                    return c2;
                }
                else
                {
                    return c1;
                }
            }

            [[nodiscard]] constexpr auto choose_2(const c_r_type c1, const c_r_type c2) const noexcept
            {
                if constexpr (is_auto_ordered)
                {
                    if (c1 < c2)
                    {
                        return c2;
                    }
                    return c1;
                }
                else
                {
                    return c2;
                }
            }

           public:
            ///////////////////////////////////////////////////////////////////////////////////////
            // CTORS
            ///////////////////////////////////////////////////////////////////////////////////////

            // we only allow valid objects
            hand_helper() = delete;

            // create from two cards/ranks, may throw depending on template
            constexpr hand_helper(const c_r_type c1, const c_r_type c2) noexcept(allow_duplicates)
                : m_card1(choose_1(c1, c2)), m_card2(choose_2(c1, c2))
            {
                if constexpr (!allow_duplicates)
                {
                    if (c1 == c2)
                    {
                        throw std::runtime_error("CTOR hand: tried to create hand with equal input (no duplicates allowed): " + c1.str() +
                                                 c2.str());
                    }
                }
            }

            // fast CTOR for convenience, may throw
            //constexpr hand_helper(const int8_t i1, const int8_t i2) : hand_helper(c_r_type(i1), c_r_type(i2)) {}

            // create from string, may throw
            constexpr explicit hand_helper(const std::string_view str)
                : hand_helper(c_r_type{str.substr(0, char_count)}, c_r_type{str.substr(char_count, char_count)})
            {
                if (str.size() != 2 * char_count)
                {
                    throw std::runtime_error(
                        std::string("hand_helper(const string_view): tried to create a hand with a string of wrong size: '").append(str) +
                        "'");
                }
            }

            // create from cardset, enable only for card
            template <typename T = c_r_type, std::enable_if_t<is_card_v<T>, int> = 0>
            constexpr explicit hand_helper(const cardset cs)
                : hand_helper(c_r_type{static_cast<int8_t>(util::cross_idx_low64(cs.as_bitset()))},
                              c_r_type{static_cast<int8_t>(util::cross_idx_high64(cs.as_bitset()))})
            {
                if (cs.size() != 2)
                {
                    throw std::runtime_error(
                        std::string("CTOR hand: tried to create a hand with a cardset of wrong size: '").append(cs.str()) + "'");
                }
            }

            ///////////////////////////////////////////////////////////////////////////////////////
            // ACCESSORS
            ///////////////////////////////////////////////////////////////////////////////////////

            // bit representation
            [[nodiscard]] constexpr auto as_bitset() const noexcept { return m_card1.as_bitset() | m_card2.as_bitset(); }

            // get both cards/ranks
            [[nodiscard]] constexpr auto as_pair() const noexcept { return std::make_pair(m_card1, m_card2); }

            // get higher card, enable only for card
            template <typename T = c_r_type, std::enable_if_t<is_card_v<T>, int> = 0>
            [[nodiscard]] constexpr auto hi() const noexcept
            {
                if constexpr (is_auto_ordered)
                {
                    return m_card2;
                }
                else
                {
                    return m_card1 < m_card2 ? m_card2 : m_card1;
                }
            }

            // get lower card, enable only for card
            template <typename T = c_r_type, std::enable_if_t<is_card_v<T>, int> = 0>
            [[nodiscard]] constexpr auto lo() const noexcept
            {
                if constexpr (is_auto_ordered)
                {
                    return m_card1;
                }
                else
                {
                    return m_card1 < m_card2 ? m_card1 : m_card2;
                }
            }

            // return as cardset, enable only for cards
            template <typename T = c_r_type, std::enable_if_t<std::is_same_v<typename T, typename mkpoker::base::card>, int> = 0>
            [[nodiscard]] constexpr cardset to_cardset() const noexcept
            {
                return cardset(as_bitset());
            }

            // return string representation, is noexcept since we only allow valid objects to be created
            [[nodiscard]] std::string str() const noexcept { return std::string(m_card1.str() + m_card2.str()); }

            ///////////////////////////////////////////////////////////////////////////////////////
            // MUTATORS
            ///////////////////////////////////////////////////////////////////////////////////////

            // none

            ///////////////////////////////////////////////////////////////////////////////////////
            // helper functions
            ///////////////////////////////////////////////////////////////////////////////////////

            constexpr auto operator<=>(const hand_helper& h) const noexcept = default;
        };
    }    // namespace old

    /*
    namespace v1
    {
        // templated container that holds two/four cards/ranks
        // for ordinary hands and 'ranges' (with rank as value_type)
        template <typename T, std::size_t N, bool allow_duplicates, bool is_auto_ordered>
        class hand_helper
        {
            //static_assert(std::is_same_v<typename c_r_type::mkpoker_char_count::value_type, uint8_t>,
            //              "c_r_type must provide mkpoker_char_count as uint8_t");
            //constexpr static auto char_count = c_r_type::mkpoker_char_count::value;

           public:
            // encoding
            const std::array<T, N> m_arr;

            ///////////////////////////////////////////////////////////////////////////////////////
            // CTORS
            ///////////////////////////////////////////////////////////////////////////////////////

            // we only allow valid suits
            hand_helper() = delete;

            ///////////////////////////////////////////////////////////////////////////////////////
            // ACCESSORS
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
    }    // namespace v1
    */

    // create from char (acdh), can throw
    //constexpr hand_helper(const c_r_type c1, const c_r_type c2) noexcept(allow_duplicates) : m_card1(c1), m_card2(c2)
    //{
    //    if constexpr (!allow_duplicates)
    //    {
    //        if (c1 == c2)
    //        {
    //            throw std::runtime_error("CTOR hand: tried to create hand with equal input (no duplicates allowed): " + c1.str() +
    //                                     c2.str());
    //        }
    //    }

    //    if constexpr (is_auto_ordered)
    //    {
    //        if (c1 > c2)
    //        {
    //            m_card1 = c2;
    //            m_card2 = c1;
    //        }
    //    }
    //}

    //// fast CTOR for convenience, may throw
    //constexpr hand_helper(const int8_t i1, const int8_t i2) : hand_helper(c_r_type(i1), c_r_type(i2)) {}

    //// create from string, may throw
    //constexpr explicit hand_helper(const std::string_view str)
    //    : hand_helper(c_r_type{str.substr(0, char_count)}, c_r_type{str.substr(char_count, char_count)})
    //{
    //    if (str.size() != 2 * char_count)
    //    {
    //        throw std::runtime_error(std::string("CTOR hand: tried to create a hand with a string of wrong size: '").append(str) + "'");
    //    }
    //}

    //// create from cardset, enable only for card
    //template <typename T = c_r_type, std::enable_if_t<std::is_same_v<typename T::is_card_type, std::true_type>, int> = 0>
    //constexpr explicit hand_helper(const cardset cs)
    //    : hand_helper(c_r_type{static_cast<int8_t>(cross_low_rank_64(cs.code_bit()))},
    //                  c_r_type{static_cast<int8_t>(cross_high_rank_64(cs.code_bit()))})
    //{
    //    if (cs.size() != 2)
    //    {
    //        throw std::runtime_error(std::string("CTOR hand: tried to create a hand with a cardset of wrong size: '").append(cs.str()) +
    //                                 "'");
    //    }
    //}

    // hand with two cards, no duplicates allowed, automatically ordered by ascending value
    using hand_2c = hand_helper<mkpoker::base::card, false, true>;

    // 'range': hand with two ranks, duplicates are allowed, not automatically ordered,
    // combination is suited when the first rank is higher
    using hand_2r = hand_helper<mkpoker::base::rank, true, false>;

}    // namespace mkpoker::base