#pragma once

#include <mkpoker/base/rank.hpp>
#include <mkpoker/base/suit.hpp>

#include <compare>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>

namespace mkp
{
    inline namespace constants
    {
        constexpr uint8_t c_deck_size = c_num_ranks * c_num_suits;
        constexpr uint8_t c_cardindex_min = 0;
        constexpr uint8_t c_cardindex_max = c_deck_size - 1;
    }    // namespace constants

    // encodes cards in canonical order (ascending clubs,diamonds,hearts,spades) as {0...51}
    class card
    {
       public:
        // encoding
        const uint8_t m_card;

        ///////////////////////////////////////////////////////////////////////////////////////
        // CTORS
        ///////////////////////////////////////////////////////////////////////////////////////

        // we only allow valid cards
        constexpr card() = delete;

        // create from integers, 0=2c ... 51=As, can throw
        constexpr explicit card(const uint8_t idx) : m_card(idx)
        {
            if (m_card > c_cardindex_max)
            {
                throw std::runtime_error("card(const uint8_t): tried to create card with 'out of bounds' index '" + std::to_string(m_card) +
                                         "'");
            }
        }

        // create from string ("2c"..."As"), can throw
        constexpr explicit card(const std::string_view sv) : m_card(mkp::rank{sv[0]}.m_rank + mkp::suit{sv[1]}.m_suit * c_num_ranks)
        {
            if (const auto len = sv.length(); len != 2)
            {
                throw std::runtime_error(
                    std::string("card(const string_view): tried to create card with string of wrong size: '").append(sv) + "'");
            }
        }

        // create from rank and suit: noexcept since those can only ever be valid objects
        constexpr card(const rank r, const suit s) noexcept : m_card(r.m_rank + s.m_suit * c_num_ranks) {}

        // create from rank and suit as integers, can throw
        constexpr card(const rank_t rt, const suit_t st) : card(mkp::rank{rt}, mkp::suit{st}) {}

        ///////////////////////////////////////////////////////////////////////////////////////
        // ACCESSORS
        ///////////////////////////////////////////////////////////////////////////////////////

        // bit representation for cardset
        [[nodiscard]] constexpr uint64_t as_bitset() const noexcept { return uint64_t(1) << m_card; }

        // returns the rank of the card
        [[nodiscard]] constexpr mkp::rank rank() const noexcept { return mkp::rank{rank_t{uint8_t(m_card % c_num_ranks)}}; }

        // returns the suit of the card
        [[nodiscard]] constexpr mkp::suit suit() const noexcept { return mkp::suit{suit_t{uint8_t(m_card / c_num_ranks)}}; }

        // return string representation, noexcept since we only allow valid card objects to be created
        [[nodiscard]] std::string str() const noexcept { return std::string(cardstrings.substr(static_cast<size_t>(m_card) * 2, 2)); }

        ///////////////////////////////////////////////////////////////////////////////////////
        // MUTATORS
        ///////////////////////////////////////////////////////////////////////////////////////

        // none

        ///////////////////////////////////////////////////////////////////////////////////////
        // helper functions
        ///////////////////////////////////////////////////////////////////////////////////////

        // we do provide ordering, so the type can be used with set, map etc.

        constexpr std::strong_ordering operator<=>(const card& other) const noexcept
        {
            if (auto cmp = m_card % c_num_ranks <=> other.m_card % c_num_ranks; cmp != 0)
            {
                return cmp;
            }
            return m_card / c_num_suits <=> other.m_card / c_num_suits;
        }

        constexpr bool operator==(const card& other) const noexcept { return m_card == other.m_card; }

       private:
        // for string representation
        static constexpr std::string_view cardstrings =
            "2c3c4c5c6c7c8c9cTcJcQcKcAc"
            "2d3d4d5d6d7d8d9dTdJdQdKdAd"
            "2h3h4h5h6h7h8h9hThJhQhKhAh"
            "2s3s4s5s6s7s8s9sTsJsQsKsAs";
    };

    static_assert(std::is_standard_layout_v<card>, "card should have standard layout");
    static_assert(std::has_unique_object_representations_v<card>, "card should have a unique representation / be hashable");

    static_assert(!std::is_default_constructible_v<card>, "card should NOT be default constructible");

    static_assert(std::is_nothrow_constructible_v<card, rank, suit>, "card should be nothrow constructible from rank and suit");

    static_assert(std::is_trivially_copyable_v<card>, "card should be trivially & nothrow copy/move constructible");
    static_assert(std::is_trivially_copy_constructible_v<card>, "card should be trivially & nothrow copy/move constructible");
    static_assert(std::is_trivially_move_constructible_v<card>, "card should be trivially & nothrow copy/move constructible");
    static_assert(std::is_nothrow_copy_constructible_v<card>, "card should be trivially & nothrow copy/move constructible");
    static_assert(std::is_nothrow_move_constructible_v<card>, "card should be trivially & nothrow copy/move constructible");

}    // namespace mkp