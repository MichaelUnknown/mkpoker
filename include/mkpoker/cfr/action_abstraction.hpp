/*

Copyright (C) 2020 Michael Knörzer

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
    template <std::size_t N>
    struct action_abstraction_base
    {
        virtual ~action_abstraction_base() = default;

        [[nodiscard]] virtual std::vector<player_action_t> filter_actions(const gamestate<N>& gamestate) const = 0;
    };

    // sample action abstraction that allows only fold, call, raise pot and allin
    template <std::size_t N>
    struct action_abstraction_fcr final : public action_abstraction_base<N>
    {
        [[nodiscard]] virtual std::vector<player_action_t> filter_actions(const gamestate<N>& gamestate) const override
        {
            // pot-sized raise: amount to call + 100 * (amount to call + pot size) / 100
            auto pot_sized_raise = 2 * gamestate.amount_to_call() + gamestate.pot_size();

            const auto all = gamestate.possible_actions();
            std::vector<player_action_t> ret;
            std::copy_if(all.cbegin(), all.cend(), std::back_inserter(ret), [&](const player_action_t a) {
                if (a.m_action == gb_action_t::FOLD || a.m_action == gb_action_t::CALL || a.m_action == gb_action_t::ALLIN ||
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
            return ret;
        }
    };

    // sample action abstraction that allows every action
    template <std::size_t N>
    struct action_abstraction_noop final : public action_abstraction_base<N>
    {
        [[nodiscard]] virtual std::vector<player_action_t> filter_actions(const gamestate<N>& gamestate) const override
        {
            return gamestate.possible_actions();
        }
    };

}    // namespace mkp
