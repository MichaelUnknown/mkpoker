#pragma once

#include <mkpoker/base/card.hpp>
#include <mkpoker/base/cardset.hpp>

#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

namespace mkpoker::base
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

    inline namespace old
    {
        // helper class for hand_2c, hand_2r
        template <typename T, bool allow_duplicates, bool is_auto_ordered, std::enable_if_t<is_card_or_rank_v<T>, int> = 0>
        class hand_helper
        {
            using c_r_type = T;
            constexpr static auto char_count = is_card_v<c_r_type> ? 2 : 1;

            //
            // private helper functions

            [[nodiscard]] constexpr auto choose_1(const c_r_type c1, const c_r_type c2) const noexcept
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
                    return c1;
                }
            }

            [[nodiscard]] constexpr auto choose_2(const c_r_type c1, const c_r_type c2) const noexcept
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
                    return c2;
                }
            }

           public:
            //
            //  encoding

            const c_r_type m_card1;
            const c_r_type m_card2;

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
                        throw std::runtime_error(
                            "hand(const crt, const crt): tried to create hand with equal input (no duplicates allowed): " + c1.str() +
                            c2.str());
                    }
                }
            }

            // fast CTOR for convenience, may throw, enable only for card
            template <typename TT = c_r_type, std::enable_if_t<is_card_v<TT>, int> = 0>
            constexpr hand_helper(const uint8_t i1, const uint8_t i2) : hand_helper(c_r_type(i1), c_r_type(i2))
            {
            }

            // create from string, may throw
            constexpr explicit hand_helper(const std::string_view str)
                : hand_helper(c_r_type{str.substr(0 * char_count, char_count)}, c_r_type{str.substr(1 * char_count, char_count)})
            {
                if (str.size() != 2 * char_count)
                {
                    throw std::runtime_error(
                        std::string("hand(const string_view): tried to create a hand with a string of wrong size: '").append(str) + "'");
                }
            }

            // create from cardset, enable only for card
            template <typename TT = c_r_type, std::enable_if_t<is_card_v<TT>, int> = 0>
            constexpr explicit hand_helper(const cardset cs)
                : hand_helper(c_r_type{static_cast<uint8_t>(util::cross_idx_low64(cs.as_bitset()))},
                              c_r_type{static_cast<uint8_t>(util::cross_idx_high64(cs.as_bitset()))})
            {
                if (cs.size() != 2)
                {
                    throw std::runtime_error(
                        std::string("hand(const cardset): tried to create a hand with a cardset of wrong size: '").append(cs.str()) + "'");
                }
            }

            ///////////////////////////////////////////////////////////////////////////////////////
            // ACCESSORS
            ///////////////////////////////////////////////////////////////////////////////////////

            // bit representation
            [[nodiscard]] constexpr auto as_bitset() const noexcept { return m_card1.as_bitset() | m_card2.as_bitset(); }

            // get both cards/ranks
            [[nodiscard]] constexpr auto as_pair() const noexcept { return std::make_pair(m_card1, m_card2); }

            // return as cardset, enable only for cards
            template <typename TT = c_r_type, std::enable_if_t<is_card_v<TT>, int> = 0>
            [[nodiscard]] constexpr cardset as_cardset() const noexcept
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

    namespace v1
    {
        // helper class for hand_2c, hand_2r
        template <typename T, bool allow_duplicates, bool is_auto_ordered, std::size_t N, std::enable_if_t<is_card_or_rank_v<T>, int> = 0>
        class hand_helper
        {
            static_assert(N >= 2 || N <= 4, "for now, only handsizes of 2 <= N <= 4 are supported");

            using c_r_type = T;
            constexpr static auto char_count = is_card_v<c_r_type> ? 2 : 1;

            // private helper to combine bitsets
            template <std::size_t... I>
            [[nodiscard]] constexpr uint64_t as_bitset_impl(std::index_sequence<I...>) const noexcept
            {
                return (m_arr[I].as_bitset() | ...);
            }

            // private helper to make a tuple
            template <std::size_t... I>
            [[nodiscard]] constexpr uint64_t as_tuple_impl(std::index_sequence<I...>) const noexcept
            {
                return std::make_tuple(m_arr[I]...);
            }

           public:
            // encoding
            const std::array<T, N> m_arr;

            ///////////////////////////////////////////////////////////////////////////////////////
            // CTORS
            ///////////////////////////////////////////////////////////////////////////////////////

            // we only allow valid objects
            hand_helper() = delete;

            // create from two cards/ranks, may throw depending on template
            template <std::enable_if_t<N == 2, int> = 0>
            constexpr hand_helper(const c_r_type c1, const c_r_type c2) noexcept(allow_duplicates) : m_arr({c1, c2})
            {
                if constexpr (!allow_duplicates)
                {
                    if (c1 == c2)
                    {
                        throw std::runtime_error(
                            "hand(const crt, const crt): tried to create hand with equal input (no duplicates allowed): " + c1.str() +
                            c2.str());
                    }
                }
            }

            // fast CTOR for convenience, may throw, enable only for card
            template <std::enable_if_t<is_card_v<T> && N == 2, int> = 0>
            constexpr hand_helper(const uint8_t i1, const uint8_t i2) : hand_helper(c_r_type(i1), c_r_type(i2))
            {
            }

            // create from string, may throw
            template <std::enable_if_t<N == 2, int> = 0>
            constexpr explicit hand_helper(const std::string_view str)
                : hand_helper(c_r_type{str.substr(0 * char_count, char_count)}, c_r_type{str.substr(1 * char_count, char_count)})
            {
                if (str.size() != 2 * char_count)
                {
                    throw std::runtime_error(
                        std::string("hand(const string_view): tried to create a hand with a string of wrong size: '").append(str) + "'");
                }
            }

            // create from cardset, enable only for card
            template <typename TT = c_r_type, std::enable_if_t<is_card_v<TT> && N == 2, int> = 0>
            constexpr explicit hand_helper(const cardset cs)
                : hand_helper(c_r_type{static_cast<uint8_t>(util::cross_idx_low64(cs.as_bitset()))},
                              c_r_type{static_cast<uint8_t>(util::cross_idx_high64(cs.as_bitset()))})
            {
                if (cs.size() != 2)
                {
                    throw std::runtime_error(
                        std::string("hand(const cardset): tried to create a hand with a cardset of wrong size: '").append(cs.str()) + "'");
                }
            }

            ///////////////////////////////////////////////////////////////////////////////////////
            // ACCESSORS
            ///////////////////////////////////////////////////////////////////////////////////////

            // return bit code
            template <typename Indices = std::make_index_sequence<N>>
            [[nodiscard]] constexpr auto as_bitset() const noexcept
            {
                return as_bitset_impl(Indices{});
            }

            // get pair of cards/ranks if N=2
            template <std::enable_if_t<N == 2, int> = 0>
            [[nodiscard]] constexpr auto as_pair() const noexcept
            {
                return std::make_pair(m_arr[0], m_arr[1]);
            }

            // get tuple of cards/ranks if N>2
            template <typename Indices = std::make_index_sequence<N>, std::enable_if_t<(N > 2), int> = 0>
            [[nodiscard]] constexpr auto as_tuple() const noexcept
            {
                as_tuple_impl(Indices{});
            }

            // return as cardset, enable only for cards
            template <typename TT = c_r_type, std::enable_if_t<is_card_v<TT>, int> = 0>
            [[nodiscard]] constexpr cardset as_cardset() const noexcept
            {
                return cardset(as_bitset());
            }

            // return string representation, is noexcept since we only allow valid objects to be created
            template <std::enable_if_t<N == 2, int> = 0>
            [[nodiscard]] std::string str() const noexcept
            {
                return std::string(m_arr.front().str() + m_arr.back().str());
            }

            ///////////////////////////////////////////////////////////////////////////////////////
            // MUTATORS
            ///////////////////////////////////////////////////////////////////////////////////////

            // none

            ///////////////////////////////////////////////////////////////////////////////////////
            // helper functions
            ///////////////////////////////////////////////////////////////////////////////////////

            constexpr auto operator<=>(const hand_helper&) const noexcept = default;
        };
    }    // namespace v1

    // hand with two cards, no duplicates allowed, automatically ordered by ascending value
    using hand_2c = hand_helper<mkpoker::base::card, false, true>;
    //using hand_2c = v1::hand_helper<mkpoker::base::card, false, true, 2>;
    //using hand_4c = v1::hand_helper<mkpoker::base::card, false, true, 4>;

    // part of a 'range': hand with two ranks, duplicates are allowed, not automatically ordered,
    // part is suited when the first rank is higher
    using hand_2r = hand_helper<mkpoker::base::rank, true, false>;
    //using hand_2r = v1::hand_helper<mkpoker::base::card, true, false, 2>;
    //using hand_4r = v1::hand_helper<mkpoker::base::card, true, false, 4>;

}    // namespace mkpoker::base