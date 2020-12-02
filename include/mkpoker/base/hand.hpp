#pragma once

#include <mkpoker/base/card.hpp>
#include <mkpoker/base/cardset.hpp>

#include <array>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

namespace mkp
{
    //
    // some TMP helpers

    template <typename T>
    using is_card = std::is_same<T, typename mkp::card>;
    template <typename T>
    constexpr bool is_card_v = is_card<T>::value;
    template <typename T>
    using is_rank = std::is_same<T, typename mkp::rank>;
    template <typename T>
    inline constexpr bool is_rank_v = is_rank<T>::value;
    template <typename T>
    using is_card_or_rank = std::disjunction<is_card<T>, is_rank<T>>;
    template <typename T>
    inline constexpr bool is_card_or_rank_v = is_card_or_rank<T>::value;

    inline namespace v0
    {
        // helper class for hand_2c, hand_2r
        template <typename T, bool allow_duplicates, bool is_auto_ordered, std::enable_if_t<is_card_or_rank_v<T>, int> = 0>
        class hand_helper
        {
            using c_r_type = T;
            constexpr static auto char_count = is_card_v<c_r_type> ? 2 : 1;

            ///////////////////////////////////////////////////////////////////////////////////////
            // private helper functions
            ///////////////////////////////////////////////////////////////////////////////////////

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

            // fast CTOR for convenience (card), may throw
            template <typename U = c_r_type, std::enable_if_t<is_card_v<U>, int> = 0>
            constexpr hand_helper(const uint8_t ui1, const uint8_t ui2) : hand_helper(c_r_type(ui1), c_r_type(ui2))
            {
            }

            // fast CTOR for convenience (card), may throw
            template <typename U = c_r_type, std::enable_if_t<is_rank_v<U>, int> = 0>
            constexpr hand_helper(const uint8_t ui1, const uint8_t ui2) : hand_helper(c_r_type(rank_t(ui1)), c_r_type(rank_t(ui2)))
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
            template <typename U = c_r_type, std::enable_if_t<is_card_v<U>, int> = 0>
            constexpr explicit hand_helper(const cardset cs)
                : hand_helper(c_r_type{static_cast<uint8_t>(cross_idx_low64(cs.as_bitset()))},
                              c_r_type{static_cast<uint8_t>(cross_idx_high64(cs.as_bitset()))})
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
            template <typename U = c_r_type, std::enable_if_t<is_card_v<U>, int> = 0>
            [[nodiscard]] constexpr cardset as_cardset() const noexcept
            {
                return cardset({m_card1, m_card2});
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
    }    // namespace v0

    namespace v1
    {
        //
        // note: not implemented yet, will not compile / fail tests

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
            template <typename U = c_r_type, std::enable_if_t<is_card_v<U> && N == 2, int> = 0>
            constexpr explicit hand_helper(const cardset cs)
                : hand_helper(c_r_type{static_cast<uint8_t>(cross_idx_low64(cs.as_bitset()))},
                              c_r_type{static_cast<uint8_t>(cross_idx_high64(cs.as_bitset()))})
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
            template <typename U = c_r_type, std::enable_if_t<is_card_v<U>, int> = 0>
            [[nodiscard]] constexpr cardset as_cardset() const noexcept
            {
                return cardset(m_arr);
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
    using hand_2c = hand_helper<card, false, true>;
    //using hand_2c = v1::hand_helper<card, false, true, 2>;
    //using hand_4c = v1::hand_helper<card, false, true, 4>;

    // part of a 'range': hand with two ranks, duplicates are allowed, not automatically ordered,
    // part is suited when the first rank is higher
    using hand_2r = hand_helper<rank, true, false>;
    //using hand_2r = v1::hand_helper<rank, true, false, 2>;

#if !defined(__clang__)
    // clang 11 seems to differ with msvc and gcc about these asserts :(

    // checks for hand_2c
    static_assert(std::is_standard_layout_v<hand_2c>, "hand_2c should have standard layout");
    static_assert(std::has_unique_object_representations_v<hand_2c>, "hand_2c should have a unique representation / be hashable");

    static_assert(!std::is_default_constructible_v<hand_2c>, "hand_2c should NOT be default constructible");

    static_assert(std::is_trivially_copyable_v<hand_2c>, "hand_2c should be trivially & nothrow copy/move constructible");
    static_assert(std::is_trivially_copy_constructible_v<hand_2c>, "hand_2c should be trivially & nothrow copy/move constructible");
    static_assert(std::is_trivially_move_constructible_v<hand_2c>, "hand_2c should be trivially & nothrow copy/move constructible");
    static_assert(std::is_nothrow_copy_constructible_v<hand_2c>, "hand_2c should be trivially & nothrow copy/move constructible");
    static_assert(std::is_nothrow_move_constructible_v<hand_2c>, "hand_2c should be trivially & nothrow copy/move constructible");

    // checks for hand_2r
    static_assert(std::is_standard_layout_v<hand_2r>, "hand_2r should have standard layout");
    static_assert(std::has_unique_object_representations_v<hand_2r>, "hand_2r should have a unique representation / be hashable");

    static_assert(!std::is_default_constructible_v<hand_2r>, "hand_2r should NOT be default constructible");

    static_assert(std::is_nothrow_constructible_v<hand_2r, rank, rank>, "hand_2r should be nothrow constructible from two ranks");

    static_assert(std::is_trivially_copyable_v<hand_2r>, "hand_2r should be trivially & nothrow copy/move constructible");
    static_assert(std::is_trivially_copy_constructible_v<hand_2r>, "hand_2r should be trivially & nothrow copy/move constructible");
    static_assert(std::is_trivially_move_constructible_v<hand_2r>, "hand_2r should be trivially & nothrow copy/move constructible");
    static_assert(std::is_nothrow_copy_constructible_v<hand_2r>, "hand_2r should be trivially & nothrow copy/move constructible");
    static_assert(std::is_nothrow_move_constructible_v<hand_2r>, "hand_2r should be trivially & nothrow copy/move constructible");
#endif

}    // namespace mkp