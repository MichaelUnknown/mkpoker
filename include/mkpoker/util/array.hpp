#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <functional>

namespace mkp
{
    template <typename T, std::size_t N>
    constexpr std::array<T, N>& operator+=(std::array<T, N>& lhs, const std::array<T, N>& rhs)
    {
        for (std::size_t i = 0; i < N; ++i)
            lhs[i] += rhs[i];
        return lhs;
    }

    template <typename T, std::size_t N>
    constexpr std::array<T, N> operator+(const std::array<T, N>& lhs, const std::array<T, N>& rhs)
    {
        std::array<T, N> result = lhs;
        result += rhs;
        return result;
    }
}    // namespace mkp
