/*

Copyright (C) Michael Knörzer

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

// from https://stackoverflow.com/questions/60802864/emulating-gccs-builtin-unreachable-in-visual-studio
// as stand-in until std::unreachable() is available in the STL

namespace mkp
{
#ifdef __GNUC__    // GCC 4.8+, Clang, Intel and other compilers compatible with GCC (-std=c++0x or above)
    [[noreturn]] inline __attribute__((always_inline)) void unreachable() { __builtin_unreachable(); }    // LCOV_EXCL_LINE
#elif defined(_MSC_VER)    // MSVC
    [[noreturn]] __forceinline void unreachable() { __assume(false); }
#else                      // ???
    inline void unreachable() {}
#endif
}    // namespace mkp

// no constexpr std::string in clang yet
#ifdef __clang__
#define MKP_CONSTEXPR_STD_STR
#else
#define MKP_CONSTEXPR_STD_STR constexpr
#endif