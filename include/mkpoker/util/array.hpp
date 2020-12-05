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
        std::transform(lhs.begin(), lhs.end(), rhs.begin(), lhs.begin(), std::plus{});
        return lhs;
    }

    template <typename T, std::size_t N>
    constexpr std::array<T, N> operator+(const std::array<T, N>& lhs, const std::array<T, N>& rhs)
    {
        std::array<T, N> res;
        for (unsigned i = 0; i < N; ++i)
            res[i] = lhs[i] + rhs[i];
        return res;
    }
}    // namespace mkp
