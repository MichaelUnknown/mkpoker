#pragma once

#include <mkpoker/base/cardset.hpp>
#include <mkpoker/base/hand.hpp>
#include <mkpoker/holdem/holdem_evaluation_result.hpp>

#include <stdexcept>

namespace mkp
{
    // this eval assumes that the cardset contains at most 7 cards
    // otherwise, the behavior is undefined
    holdem_evaluation_result evaluate_unsafe(const mkpoker::containers::cardset cs) noexcept;

    // safe call, will check if the cardset provides a suitable cardset
    inline holdem_evaluation_result evaluate_safe(const mkpoker::containers::cardset cs)
    {
        if (const auto size = cs.size(); size() > 7 || size < 5)
        {
            throw std::runtime_error("evaluate_safe() called with cardset of wrong size: " + std::to_string(size));
        }
        return evaluate_unsafe(cs);
    }

    // variadic overload
    template <typename T, typename... TArgs>
    inline holdem_evaluation_result evaluate_safe(const mkpoker::containers::cardset cs, const T value, const TArgs... args)
    {
        return evaluate_safe(cs.combine(value), args...);
    }

}    // namespace mkp