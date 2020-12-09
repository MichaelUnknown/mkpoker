#pragma once

#include <mkpoker/base/rank.hpp>
#include <mkpoker/util/bit.hpp>

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
        constexpr uint8_t c_offset_major = c_offset_minor + 4;
        constexpr uint8_t c_offset_type = c_offset_major + 4;
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
        // encoding
        uint32_t m_result;
#if !defined(NDEBUG)
        uint8_t m_debug_type = type();
        uint8_t m_debug_major = major_rank().m_rank;
        uint8_t m_debug_minor = minor_rank().m_rank;
        //std::string m_debug_str = str().c_str();
        //std::string m_debug_bitfield = bitstr();
#endif

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
        ///////////////////////////////////////////////////////////////////////////////////////
        // CTORS
        ///////////////////////////////////////////////////////////////////////////////////////

        // we only allow valid objects
        holdem_evaluation_result() = delete;

        // we do not check if the input is valid, use with caution!
        // instead, utilize the make_he_result(...) free function to create a checked, valid result
        // todo:
        // ranks major and minor have to be given with 1 above the actual value, because otherwise we
        // could not distinguish between well-formed and invalid objects
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
            // todo: change encoding to ensure valid types
            return rank(rank_t(c_mask_ranks_numbers & ((m_result >> c_offset_major) - 0)));
        };

        // minor rank, used for full house and two pair
        [[nodiscard]] constexpr rank minor_rank() const noexcept
        {
            // todo: change encoding to ensure valid types
            return rank(rank_t(c_mask_ranks_numbers & ((m_result >> c_offset_minor) - 0)));
        };

        // up to 5 cards that are not paired, also used for all flush cards
        [[nodiscard]] constexpr uint16_t kickers() const noexcept { return (c_mask_ranks & m_result); };

        // bit representation
        [[nodiscard]] constexpr uint32_t as_bitset() const noexcept { return m_result; };

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
        [[nodiscard]] std::string bitstr() const noexcept { return std::bitset<25>(m_result).to_string(); }

        ///////////////////////////////////////////////////////////////////////////////////////
        // MUTATORS
        ///////////////////////////////////////////////////////////////////////////////////////

        // none

        ///////////////////////////////////////////////////////////////////////////////////////
        // helper functions
        ///////////////////////////////////////////////////////////////////////////////////////

        constexpr auto operator<=>(const holdem_evaluation_result&) const = default;
    };

    [[nodiscard]] constexpr auto make_he_result(uint8_t type, uint8_t major, uint8_t minor, uint16_t kickers)
    {
        // check type an major rank
        switch (type)
        {
            // should not have major rank
            case c_no_pair:
            case c_flush:
                if (major > 0)
                {
                    throw std::runtime_error("make_he_result(...): failed to create result, illegal major rank with type " +
                                             std::to_string(type) + " and major_rank " + std::to_string(major));
                }
                break;

            // should have major rank, check if valid
            case c_straight:
            case c_straight_flush:
                if (major <= c_rank_four)
                {
                    throw std::runtime_error("make_he_result(...): failed to create result, illegal major rank with type " +
                                             std::to_string(type) + " and major_rank " + std::to_string(major));
                }
                [[fallthrough]];
            case c_one_pair:
            case c_two_pair:
            case c_three_of_a_kind:
            case c_full_house:
            case c_four_of_a_kind:
                if (major > c_rank_ace)
                {
                    throw std::runtime_error("make_he_result(...): failed to create result, illegal major rank with type " +
                                             std::to_string(type) + " and major_rank " + std::to_string(major));
                }
                break;

            default:
                throw std::runtime_error("make_he_result(...): failed to create her result with invalid hand type " + std::to_string(type));
        }

        // check minor
        switch (type)
        {
            // should not have minor rank
            case c_no_pair:
            case c_one_pair:
            case c_three_of_a_kind:
            case c_straight:
            case c_flush:
            case c_four_of_a_kind:
            case c_straight_flush:
                if (minor > 0)
                {
                    throw std::runtime_error("make_he_result(...): failed to create result, illegal minor rank with type " +
                                             std::to_string(type) + " and minor_rank " + std::to_string(minor));
                }
                break;
            // should have minor rank, check if valid
            case c_two_pair:
                if (minor > major)
                {
                    throw std::runtime_error(
                        "make_he_result(...): failed to create result, minor rank must be smaller than major rank for type " +
                        std::to_string(type));
                }
                [[fallthrough]];
            case c_full_house:
                if (minor == major)
                {
                    throw std::runtime_error("make_he_result(...): failed to create result, major and minor rank must differ for type " +
                                             std::to_string(type));
                }
                if (minor > c_rank_ace)
                {
                    throw std::runtime_error("make_he_result(...): failed to create result, illegal major rank with type " +
                                             std::to_string(type) + " and minor_rank " + std::to_string(minor));
                }
                break;
        }

        auto fail_kickers = [&] {
            throw std::runtime_error("make_he_result(...): failed to create result, illegal number of kickers with type " +
                                     std::to_string(type) + " and kickers: " + std::bitset<16>(kickers).to_string());
        };

        // check kickers
        switch (type)
        {
            // should have a specific number of kickers
            // we also allow less kickers for partial evaluations (e.g. hands of 3 or 4 cards)
            case c_no_pair:
                if (std::popcount(kickers) > 5)
                {
                    fail_kickers();
                }
                break;
            case c_one_pair:
                if (std::popcount(kickers) > 3 || uint16_t(1 << major) & kickers)
                {
                    fail_kickers();
                }
                break;
            case c_two_pair:
                if (std::popcount(kickers) > 1 || (uint16_t(1 << major) | uint16_t(1 << minor)) & kickers)
                {
                    fail_kickers();
                }
                break;
            case c_three_of_a_kind:
                if (std::popcount(kickers) > 2 || uint16_t(1 << major) & kickers)
                {
                    fail_kickers();
                }
                break;
            case c_flush:
                if (std::popcount(kickers) > 5)
                {
                    fail_kickers();
                }
                break;
            case c_four_of_a_kind:
                if (std::popcount(kickers) > 1 || uint16_t(1 << major) == kickers)
                {
                    fail_kickers();
                }
                break;

            // should not have kickers
            case c_straight:
            case c_full_house:
            case c_straight_flush:
                if (std::popcount(kickers) > 0)
                {
                    fail_kickers();
                }
                break;
        }

        return holdem_evaluation_result(type, major, minor, kickers);
    }

}    // namespace mkp