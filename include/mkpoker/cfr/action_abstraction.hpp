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

        virtual [[nodiscard]] std::vector<player_action_t> filter_actions(const gamestate<N>& gamestate) const = 0;
    };

    // sample action abstraction that allows only fold, call, raise pot and allin
    template <std::size_t N>
    struct action_abstraction_fcr final : public action_abstraction_base<N>
    {
        virtual [[nodiscard]] std::vector<player_action_t> filter_actions(const gamestate<N>& gamestate) const override
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
        virtual [[nodiscard]] std::vector<player_action_t> filter_actions(const gamestate<N>& gamestate) const override
        {
            return gamestate.possible_actions();
        }
    };

}    // namespace mkp
