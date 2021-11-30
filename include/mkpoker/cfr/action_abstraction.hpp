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

#include <mkpoker/base/rank.hpp>
#include <mkpoker/game/game.hpp>

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <string>
#include <vector>

namespace mkp
{
    template <typename T>
    struct action_abstraction_base
    {
        using game_type = T;

        virtual ~action_abstraction_base() = default;

        // restricts the list of possible actions
        [[nodiscard]] virtual std::vector<player_action_t> filter_actions(const game_type& gamestate) const = 0;
    };

    // action abstraction that allows every action
    template <typename T>
    struct action_abstraction_noop final : public action_abstraction_base<T>
    {
        using typename action_abstraction_base<T>::game_type;

        [[nodiscard]] virtual std::vector<player_action_t> filter_actions(const game_type& gamestate) const override
        {
            return gamestate.possible_actions();
        }
    };

    // a simple abstraction that allows preflop actions (fold, call, raise pot and all in) and 'checking down to the river',
    // i.e., no postflop actions
    template <typename T>
    struct action_abstraction_simple_preflop final : public action_abstraction_base<T>
    {
        using typename action_abstraction_base<T>::game_type;

        [[nodiscard]] virtual std::vector<player_action_t> filter_actions(const game_type& gamestate) const override
        {
            std::vector<player_action_t> ret;

            if (gamestate.gamestate_v() == gb_gamestate_t::PREFLOP_BET)
            {
                // XX%-sized raise: amount to call + XX * (amount to call + pot size) / 100
                // pot-sized raise: amount to call + amount to call + pot size
                auto pot_sized_raise = 2 * gamestate.amount_to_call() + gamestate.pot_size();

                const auto all = gamestate.possible_actions();
                std::copy_if(all.cbegin(), all.cend(), std::back_inserter(ret), [&](const player_action_t a) {
                    if (a.m_action == gb_action_t::FOLD || a.m_action == gb_action_t::CALL || a.m_action == gb_action_t::CHECK ||
                        a.m_action == gb_action_t::ALLIN ||
                        (a.m_action == gb_action_t::RAISE &&
                         (std::abs(a.m_amount - pot_sized_raise) < 250 || a.m_amount - pot_sized_raise == 250)))
                    {
                        return true;
                    }
                    else
                    {
                        return false;
                    }
                });
            }
            else
            {
                ret.emplace_back(0, gb_action_t::CHECK, gamestate.active_player_v());
            }
            return ret;
        }
    };

}    // namespace mkp
