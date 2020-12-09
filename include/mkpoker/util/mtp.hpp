#pragma once

#include <array>
#include <cstddef>       // std::size_t
#include <functional>    // std::identity
#include <utility>       // std::integer_sequence, std::make_index_sequence, std::forward

namespace mkp
{
    namespace detail
    {
        // use index_sequence / parameter packs to make the array, set to constant value
        template <typename T, std::size_t... I>
        constexpr auto array_impl(const T& val, const std::index_sequence<I...>) noexcept -> typename std::array<T, sizeof...(I)>
        {
            auto f = [&](std::size_t) { return val; };
            return {f(I)...};
        }

        // use index_sequence / parameter packs to make the array, init with function object
        template <typename T, std::size_t... I, typename F>
        constexpr auto array_impl_fn(const F& f, const std::index_sequence<I...>) noexcept -> typename std::array<T, sizeof...(I)>
        {
            return {static_cast<T>(f(I))...};
        }

    }    // namespace detail

    // identity fix for libc++
#if defined(_LIBCPP_VERSION) && (_LIBCPP_VERSION <= 12000)
    struct identity
    {
        template <typename T>
        [[nodiscard]] constexpr T&& operator()(T&& t) const noexcept
        {
            return std::forward<T>(t);
        }
    };
#else
    using identity = std::identity;
#endif

    // make an array of size N, initialize with val
    template <std::size_t N, typename T>
    constexpr auto make_array(const T& val) noexcept
    {
        using Sequence = std::make_index_sequence<N>;
        return detail::array_impl(val, Sequence{});
    }

    // make an array of size N, initialize with function object
    template <typename T, std::size_t N, typename F>
    constexpr auto make_array(const F& f) noexcept
    {
        using Sequence = std::make_index_sequence<N>;
        return detail::array_impl_fn<T>(f, Sequence{});
    }

}    // namespace mkp
