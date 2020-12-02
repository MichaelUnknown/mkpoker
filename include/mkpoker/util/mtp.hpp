#pragma once

#include <array>
#include <cstddef>
#include <type_traits>
#include <utility>

namespace mkp
{
    namespace detail
    {
        // use index_sequence / parameter packs to make the array, set to constant value
        template <typename T, std::size_t... I>
        constexpr auto array_impl(const T& val, const std::index_sequence<I...>) -> typename std::array<T, sizeof...(I)>
        {
            auto f = [&](std::size_t) { return val; };
            return {f(I)...};
        }

        // use index_sequence / parameter packs to make the array, init with projection
        template <typename T, std::size_t... I, typename P>
        constexpr auto array_impl_fn(const P& p, const std::index_sequence<I...>) -> typename std::array<T, sizeof...(I)>
        {
            return {static_cast<T>(p(I))...};
        }

    }    // namespace detail

    // make an array of size N, initialize with val
    template <std::size_t N, typename T>
    constexpr auto make_array(const T& val)
    {
        using Sequence = std::make_index_sequence<N>;
        return detail::array_impl(val, Sequence{});
    }

    // make an array of size N, initialize with projection
    template <typename T, std::size_t N, typename P>
    constexpr auto make_array(const P& p)
    {
        using Sequence = std::make_index_sequence<N>;
        return detail::array_impl_fn<T>(p, Sequence{});
    }

}    // namespace mkp
