#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>

namespace mkp
{
    inline namespace constants
    {
        // positions
        enum class gb_pos_t : uint8_t
        {
            SB = 0,
            BB,
            UTG,
            MP,
            CO,
            BTN
        };

        // possible game states
        enum class gb_gamestate_t : uint8_t
        {
            PREFLOP_BET = 0,
            FLOP_BET,
            TURN_BET,
            RIVER_BET,
            GAME_FIN
        };

        // possible player states
        enum class gb_playerstate_t : uint8_t
        {
            INIT = 0,
            OUT,
            ALIVE,
            ALLIN
        };

        // all possible actions (F/X/C/R/A)
        enum class gb_action_t : uint8_t
        {
            FOLD = 0,
            CHECK,
            CALL,
            RAISE,
            ALLIN
        };

        ///////////////////////////////////////////////////////////////////////////////////////
        // string conversions
        ///////////////////////////////////////////////////////////////////////////////////////

        [[nodiscard]] std::string to_string(const gb_gamestate_t gs)
        {
            switch (gs)
            {
                case gb_gamestate_t::PREFLOP_BET:
                    return std::string("PREFLOP_BET");
                case gb_gamestate_t::FLOP_BET:
                    return std::string("FLOP_BET");
                case gb_gamestate_t::TURN_BET:
                    return std::string("TURN_BET");
                case gb_gamestate_t::RIVER_BET:
                    return std::string("RIVER_BET");
                case gb_gamestate_t::GAME_FIN:
                    return std::string("GAME_FIN");

                default:
                    throw std::runtime_error("to_string(const gb_gamestate_t): invalid game state " +
                                             std::to_string(static_cast<std::underlying_type_t<decltype(gs)>>(gs)));
            }
        }

        [[nodiscard]] std::string to_string(const gb_playerstate_t ps)
        {
            switch (ps)
            {
                case gb_playerstate_t::INIT:
                    return std::string("INIT");
                case gb_playerstate_t::OUT:
                    return std::string("OUT");
                case gb_playerstate_t::ALIVE:
                    return std::string("ALIVE");
                case gb_playerstate_t::ALLIN:
                    return std::string("ALLIN");

                default:
                    throw std::runtime_error("to_string(const gb_playerstate_t): invalid player state " +
                                             std::to_string(static_cast<std::underlying_type_t<decltype(ps)>>(ps)));
            }
        }

        [[nodiscard]] std::string to_string(const gb_action_t a)
        {
            switch (a)
            {
                case gb_action_t::FOLD:
                    return std::string("FOLD");
                case gb_action_t::CHECK:
                    return std::string("CHECK");
                case gb_action_t::CALL:
                    return std::string("CALL");
                case gb_action_t::RAISE:
                    return std::string("RAISE");
                case gb_action_t::ALLIN:
                    return std::string("ALLIN");

                default:
                    throw std::runtime_error("to_string(const gb_playerstate_t): invalid player state " +
                                             std::to_string(static_cast<std::underlying_type_t<decltype(a)>>(a)));
            }
        }

    }    // namespace constants

    // struct for player actions
    struct player_action_t
    {
        int32_t m_amount;
        gb_action_t m_action;
        gb_pos_t m_pos;

        [[nodiscard]] std::string str() const { return to_string(m_action).append("(" + std::to_string(m_amount) + ")"); }

        constexpr auto operator<=>(const player_action_t&) const noexcept = delete;
        constexpr bool operator==(const player_action_t&) const noexcept = default;
    };
}    // namespace mkp