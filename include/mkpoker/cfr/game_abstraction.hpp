#pragma once

#include <mkpoker/game/game.hpp>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace mkp
{
    template <std::size_t N, typename T = uint32_t>
    struct game_abstraction_base
    {
        using uint_type = T;

        virtual ~game_abstraction_base() = default;

        virtual uint_type encode(const gamestate<N>& gamestate) = 0;
        virtual [[nodiscard]] gamestate<N> decode(const uint_type i) const = 0;
    };

    // sample encoder that stores / enumerates the gamestates
    template <std::size_t N, typename T = uint32_t>
    struct gamestate_enumerator final : public game_abstraction_base<N, T>
    {
        using game_abstraction_base<N, T>::uint_type;

        uint_type index = 0;
        std::vector<gamestate<N>> storage = {};

        virtual uint_type encode(const gamestate<N>& gamestate) override
        {
            storage.push_back(gamestate);
            return index++;
        }

        virtual [[nodiscard]] gamestate<N> decode(const uint_type i) const override { return storage.at(i); }
    };

    // sample encoder that stores / enumerates the gamestates
    template <std::size_t N, typename T = uint32_t>
    struct gamestate_discarder final : public game_abstraction_base<N, T>
    {
        using game_abstraction_base<N, T>::uint_type;

        uint_type index = 0;

        virtual uint_type encode([[maybe_unused]] const gamestate<N>& gamestate) override { return index++; }

        virtual [[nodiscard]] gamestate<N> decode([[maybe_unused]] const uint_type i) const override
        {
            throw std::runtime_error("gamestate_discarder does not provide a decode function");
        }
    };

}    // namespace mkp
