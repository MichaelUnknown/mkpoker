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

#include <cstdint>
#include <stdexcept>
#include <string>

namespace mkp
{
    inline namespace constants
    {
        constexpr uint8_t c_num_board_cards = 5;

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

    }    // namespace constants

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

    // struct for player actions
    struct player_action_t
    {
        int32_t m_amount;
        gb_action_t m_action;
        gb_pos_t m_pos;

        player_action_t() = default;
        constexpr player_action_t(int32_t amount, gb_action_t action, gb_pos_t pos) noexcept
            : m_amount(amount), m_action(action), m_pos(pos){};

        [[nodiscard]] std::string str() const { return to_string(m_action).append("(" + std::to_string(m_amount) + ")"); }

        constexpr auto operator<=>(const player_action_t&) const noexcept = delete;
        constexpr bool operator==(const player_action_t&) const noexcept = default;
    };

    static_assert(std::is_standard_layout_v<player_action_t>, "player_action_t should have standard layout");
    static_assert(std::is_trivial_v<player_action_t>, "player_action_t should be trivial");

    static_assert(std::is_nothrow_constructible_v<player_action_t, int32_t, gb_action_t, gb_pos_t>,
                  "player_action_t should be nothrow constructible from int32_t, gb_action_t and gb_pos_t");

    static_assert(std::is_trivially_copyable_v<player_action_t>, "player_action_t should be trivially & nothrow copy/move constructible");
    static_assert(std::is_trivially_copy_constructible_v<player_action_t>,
                  "player_action_t should be trivially & nothrow copy/move constructible");
    static_assert(std::is_trivially_move_constructible_v<player_action_t>,
                  "player_action_t should be trivially & nothrow copy/move constructible");
    static_assert(std::is_nothrow_copy_constructible_v<player_action_t>,
                  "player_action_t should be trivially & nothrow copy/move constructible");
    static_assert(std::is_nothrow_move_constructible_v<player_action_t>, "card should be trivially & nothrow copy/move constructible");
}    // namespace mkp