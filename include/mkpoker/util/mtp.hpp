/*

Copyright (C) 2020 Michael Knörzer

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as
published by the Free Software Foundation, either version 3 of the
License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#pragma once

#include <array>
#include <cstddef>        // std::size_t
#include <functional>     // std::identity
#include <type_traits>    // std::is_integral etc.
#include <utility>        // std::integer_sequence, std::make_index_sequence, std::forward

namespace mkp
{
    template <class T>
    concept UnsignedIntegral = (std::is_integral<T>::value) && (std::is_unsigned<T>::value);

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
