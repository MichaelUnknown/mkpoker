#pragma once

#include <mkpoker/base/rank.hpp>
#include <mkpoker/game/game.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace mkp
{
    template <std::size_t N, typename T = uint32_t>
    struct card_abstraction_base
    {
        using uint_type = T;

        virtual ~card_abstraction_base() = default;

        [[nodiscard]] virtual uint_type size(const gb_gamestate_t game_state) const = 0;
        [[nodiscard]] virtual uint_type id(const gb_gamestate_t game_state, const gamecards<N>& cards) const = 0;

        // debug / human readable description of that abstraction
        [[nodiscard]] virtual std::string str_id(const gb_gamestate_t game_state, uint_type id) const = 0;
    };

    // sample card abstraction that buckets cards by range classification, i.e. AA, AKs, AKo etc.
    template <std::size_t N, typename T = uint32_t>
    struct card_abstraction_by_range final : public card_abstraction_base<N, T>
    {
        using card_abstraction_base<N, T>::uint_type;

        [[nodiscard]] virtual uint_type size(const gb_gamestate_t game_state) const override { return c_num_ranks * c_num_ranks; }

        [[nodiscard]] virtual uint_type id(const gb_gamestate_t game_state, const gamecards<N>& cards) const override
        {
            get_index(cards.get_hand(state.active_player()));
        }

        // debug / human readable description of that id
        [[nodiscard]] virtual std::string str_id(const gb_gamestate_t game_state, uint_type id) const override
        {
            return str_hand_2r(range::get_hand(static_cast<uint8_t>(id))) + " (" + std::to_string(id) + ")";
        }
    };

}    // namespace mkp
