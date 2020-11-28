#pragma once

#include <mkpoker/base/rank.hpp>

#include <array>
#include <bitset>
#include <cstdint>
#include <stdexcept>
#include <string>

namespace mkp
{
    inline namespace consts
    {
        constexpr uint8_t c_no_pair = 0;
        constexpr uint8_t c_one_pair = 1;
        constexpr uint8_t c_two_pair = 2;
        constexpr uint8_t c_three_of_a_kind = 3;
        constexpr uint8_t c_straight = 4;
        constexpr uint8_t c_flush = 5;
        constexpr uint8_t c_full_house = 6;
        constexpr uint8_t c_four_of_a_kind = 7;
        constexpr uint8_t c_straight_flush = 8;

        constexpr uint8_t c_mask_type = 0b0000'1111;

        constexpr uint8_t c_offset_minor = c_num_ranks;
        constexpr uint8_t c_offset_major = c_num_ranks + 4;
        constexpr uint8_t c_offset_type = c_num_ranks + 8;
    }    // namespace consts

    // size: 32 bits
    //
    // since a hand can have more than one kicker (e.g., in case of no pair), we represent each kicker with a bit
    // the rest is encoded as enum of (T)ype and rank (M)ajor/(m)inor, 4 bits each
    // the order is so that ranks work out of the box with comparison operators
    // a result can only be constructed once and not changed
    //
    // 0987654 32109876 54321098 76543210
    //       T TTTMMMMm mmmkkkkk kkkkkkkk
    class holdem_evaluation_result
    {
        ///////////////////////////////////////////////////////////////////////////////////////
        // private helper functions
        ///////////////////////////////////////////////////////////////////////////////////////

        // string representation (ranks) of all kickers
        std::string str_kickers() const
        {
            std::string str;
            const uint16_t k = kickers();

            int8_t i = c_num_ranks;
            for (uint64_t mask = uint64_t(1) << c_num_ranks; mask; mask >>= 1, i--)
            {
                if (k & mask)
                {
                    str += rank(i).str();
                }
            }

            return str;
        }

       public:
        // encoding
        const uint32_t m_result;

        ///////////////////////////////////////////////////////////////////////////////////////
        // CTORS
        ///////////////////////////////////////////////////////////////////////////////////////

        // we only allow valid objects
        holdem_evaluation_result() = delete;

        // we do not check if the input is valid, use with caution!
        // instead, utilize the make_he_result(...) free function to create a checked, valid result
        constexpr holdem_evaluation_result(uint8_t type, uint8_t major, uint8_t minor, uint16_t kickers) noexcept
            : m_result((type << c_offset_type) | (major << c_offset_major) | (minor << c_offset_minor) | (kickers & c_mask_ranks))
        {
        }

        ///////////////////////////////////////////////////////////////////////////////////////
        // ACCESSORS
        ///////////////////////////////////////////////////////////////////////////////////////

        // type of the result, e.g., one pair, full house etc.
        [[nodiscard]] constexpr uint8_t type() const noexcept { return static_cast<uint8_t>(m_result >> c_offset_type); };

        // major rank, used for pair(s), full house and straight (flush)
        [[nodiscard]] constexpr rank major_rank() const noexcept
        {
            return rank(rank_t{c_mask_ranks_numbers & (m_result >> c_offset_major)});
        };

        // minor rank, used for full house and two pair
        [[nodiscard]] constexpr rank minor_rank() const noexcept
        {
            return rank(rank_t{c_mask_ranks_numbers & (m_result >> c_offset_minor)});
        };

        // up to 5 cards that are not paired, also used for all flush cards
        [[nodiscard]] constexpr uint16_t kickers() const noexcept { return (c_mask_ranks & m_result); };

        // string representation, e.g. "flush: Ace high"
        [[nodiscard]] std::string str() const
        {
            static const std::array<std::string, 9> str_representation{
                "high card:    ", "one pair:     ", "two pair:     ", "trips:        ", "straight:     ",
                "flush:        ", "full house:   ", "quads:        ", "str8 flush:   "};

            switch (const uint8_t t = type(); t)
            {
                case c_no_pair:
                    return str_representation[t] + str_kickers();
                case c_one_pair:
                case c_three_of_a_kind:
                case c_four_of_a_kind:
                    return str_representation[t] + major_rank().str() + " high, kickers: " + str_kickers();
                case c_two_pair:
                    return str_representation[t] + "(" + major_rank().str() + " & " + minor_rank().str() + "), kickers: " + str_kickers();
                case c_full_house:
                    return str_representation[t] + "(" + major_rank().str() + " & " + minor_rank().str() + ")";
                case c_straight:
                case c_straight_flush:
                    return str_representation[t] + major_rank().str() + " high";
                case c_flush:
                    return str_representation[t] + str_kickers();
                default:
                    return "invalid evaluation result";
            }
        }

        // all the bits as string
        [[nodiscard]] std::string bitstr() const { return std::bitset<25>(m_result).to_string(); }

        ///////////////////////////////////////////////////////////////////////////////////////
        // MUTATORS
        ///////////////////////////////////////////////////////////////////////////////////////

        // none

        ///////////////////////////////////////////////////////////////////////////////////////
        // helper functions
        ///////////////////////////////////////////////////////////////////////////////////////

        constexpr auto operator<=>(const holdem_evaluation_result&) const = default;
    };

    constexpr auto make_he_result(uint8_t type, uint8_t major, uint8_t minor, uint16_t kickers)
    {
        switch (type)
        {
            case c_no_pair:
                if (major > 0 || minor > 0)
                {
                    throw std::runtime_error("a");
                }
                break;
            case c_one_pair:
            case c_three_of_a_kind:
            case c_four_of_a_kind:
            case c_two_pair:
            case c_full_house:
            case c_straight:
            case c_straight_flush:
            case c_flush:
            default:
                throw std::runtime_error("make_he_result(...): failed to create her result with invalid hand type " + std::to_string(type));
        }
        return holdem_evaluation_result(type, major, minor, kickers);
    }

}    // namespace mkp