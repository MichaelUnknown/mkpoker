#pragma once

#include <array>
#include <type_traits>
#include <utility>

namespace mkp
{
    namespace detail
    {
        // use index_sequence / parameter packs to make the array, set to constant value
        template <typename T, std::size_t... I>
        constexpr auto int_array_impl(T val, std::index_sequence<I...>) -> typename std::array<T, sizeof...(I)>
        {
            auto f = [&](std::size_t) { return val; };
            return {f(I)...};
        }

        // use index_sequence / parameter packs to make the array, init with lambda
        template <typename T, std::size_t... I>
        constexpr auto int_array_impl_fn(auto f, std::index_sequence<I...>) -> typename std::array<T, sizeof...(I)>
        {
            return {f(I)...};
        }

    }    // namespace detail

    // make an array of size N, initialize with val
    template <std::size_t N, typename T>
    constexpr auto init_array(T val)
    {
        using Sequence = std::make_index_sequence<N>;
        return detail::int_array_impl(val, Sequence{});
    }

    // make an array of size N, initialize with lambda
    template <std::size_t N, typename T>
    constexpr auto init_array_fn(auto f)
    {
        using Sequence = std::make_index_sequence<N>;
        return detail::int_array_impl_fn<T>(f, Sequence{});
    }

    template <typename T>
    struct forward_as_
    {
        [[nodiscard]] constexpr T&& operator()(T&& t) const noexcept { return std::forward<T>(t); }
    };

}    // namespace mkp