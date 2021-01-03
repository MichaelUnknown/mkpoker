#pragma once

#include <mkpoker/base/rank.hpp>
#include <mkpoker/game/game.hpp>

#include <cstddef>
#include <cstdint>
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

    // sample action abstraction that allows only fold, call and raise pot
    template <std::size_t N>
    struct action_abstraction_fcr final : public action_abstraction_base<N>
    {
        virtual [[nodiscard]] std::vector<player_action_t> filter_actions(const gamestate<N>& gamestate) const override
        {
            auto all = gamestate.possible_actions();
            std::vector<player_action_t> temp;
            // do stuff
            return temp;
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
