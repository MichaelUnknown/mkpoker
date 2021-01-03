#pragma once

#include <mkpoker/game/game.hpp>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace mkp
{
    template <std::size_t N, typename T = uint32_t>
    struct gamestate_encoder_base
    {
        using uint_type = T;

        virtual ~gamestate_encoder_base() = default;

        virtual uint_type encode(const gamestate<N>& gamestate) = 0;
        virtual [[nodiscard]] gamestate<N> decode(const uint_type i) const = 0;
    };

    // sample encoder that stores / enumerates the gamestates
    template <std::size_t N, typename T = uint32_t>
    struct gamestate_enumerator final : public gamestate_encoder_base<N, T>
    {
        using gamestate_encoder_base<N, T>::uint_type;

        uint_type index = 0;
        std::vector<gamestate<N>> storage = {};

        virtual uint_type encode(const gamestate<N>& gamestate) override
        {
            storage.push_back(gamestate);
            return index++;
        }

        virtual [[nodiscard]] gamestate<N> decode(const uint_type i) const override { return storage.at(i); }
    };

}    // namespace mkp
