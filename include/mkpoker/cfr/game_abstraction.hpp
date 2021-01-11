#pragma once

#include <mkpoker/game/game.hpp>
#include <mkpoker/util/mtp.hpp>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace mkp
{
    template <std::size_t N, UnsignedIntegral T = uint32_t>
    struct game_abstraction_base
    {
        using uint_type = T;

        virtual ~game_abstraction_base() = default;

        // takes a new gamestate as input and returns the id
        virtual uint_type encode(const gamestate<N>& gamestate) = 0;

        // converts the id back to the actual gamestate
        [[nodiscard]] virtual gamestate<N> decode(const uint_type id) const = 0;
    };

    // sample encoder that stores / enumerates the gamestates
    template <std::size_t N, UnsignedIntegral T = uint32_t>
    struct gamestate_enumerator final : public game_abstraction_base<N, T>
    {
        using typename game_abstraction_base<N, T>::uint_type;

        uint_type index = 0;
        std::vector<gamestate<N>> storage = {};

        virtual uint_type encode(const gamestate<N>& gamestate) override
        {
            storage.push_back(gamestate);
            return index++;
        }

        [[nodiscard]] virtual gamestate<N> decode(const uint_type id) const override { return storage.at(id); }
    };

}    // namespace mkp
