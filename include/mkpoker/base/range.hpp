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

#include <mkpoker/base/hand.hpp>

#include <array>
#include <cstdint>
#include <numeric>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace mkp
{
    inline namespace constants
    {
        constexpr uint8_t c_range_size = c_num_ranks * c_num_ranks;
        constexpr uint8_t c_rangeindex_min = 0;
        constexpr uint8_t c_rangeindex_max = c_range_size - 1;
    }    // namespace constants

    // represents a range with a probability value in a 13x13 matrix, mapped to an array
    // pairs are on the diagonal, the suited combos on the upper/right half, non-suited below
    // range stores the probability (0..100) for each index as follows:
    // AA |AKs|AQs|... => # 0|# 1|# 2|... => AA|AK|AQ|..
    // AKo|KK |KQs|...    #13|#14|#15|...    KA|KK|KQ|..
    // AQo|KQo|QQ |...    #26|#27|#28|...    QA|QK|QQ|..
    // ...|...|...|...    ...|...|...|...    ..|..|..|..
    // 13 pairs (x6), the diagonal
    // 78 suited combinations (x4), upper triangle
    // 78 non-suited combinations (x12), lower triangle
    // = 1326 possible combinations total (sanity check: 52 choose 2 = 1326)
    class range
    {
        // internal encoding
        // we would like to be able to "round trip" between range and an explicit range
        // (i.e. vec/array of all 1326 hands) so we need to store values from 0..100
        // thus, the array member needs to hold values between 0..1200 (max factor 100x12 for nonsuited)
        std::array<uint16_t, c_range_size> m_combos = {};

        ///////////////////////////////////////////////////////////////////////////////////////
        // private helper functions
        ///////////////////////////////////////////////////////////////////////////////////////

        // parse from string
        void from_string(const std::string_view sv)
        {
            std::vector<std::string> tokens;
            std::string token;
            std::istringstream tokenStream(sv.data());
            while (std::getline(tokenStream, token, ','))
            {
                tokens.push_back(token);
            }

            //
            // helpers

            auto throw_invalid = [](std::string tok) {
                throw std::runtime_error("from_string(const string_view): could not parse token " + tok);
            };
            auto add_suited_nonsuited_asc = [this](uint8_t low, const uint8_t high, const bool is_suited) {
                while (low < high)
                {
                    if (is_suited)
                    {
                        set_value(hand_2r{high, low}, 400);
                    }
                    else
                    {
                        set_value(hand_2r{low, high}, 1200);
                    }
                    ++low;
                }
            };
            auto add_pair_asc = [this](uint8_t r) {
                for (;;)
                {
                    set_value(hand_2r{r, r}, 600);
                    if (r == c_rank_ace)
                    {
                        break;
                    }
                    ++r;
                }
            };

            // main loop
            for (const auto& tok : tokens)
            {
                // validate input
                const auto size = tok.size();
                if (size > 4 || size < 2)
                {
                    throw_invalid(tok);
                }

                try
                {
                    const auto r1 = rank(tok[0]);
                    const auto r2 = rank(tok[1]);

                    if (size == 4)
                    {
                        // "+" should be at the end, 3rd pos should be suited/nonsuited
                        // also, ranks should be different
                        if (tok[3] != '+' || (tok[2] != 's' && (tok[2] != 'o')) || r1 == r2)
                        {
                            throw_invalid(tok);
                        }

                        const bool is_suited = (tok[2] == 's');

                        if (r2 < r1)
                        {
                            add_suited_nonsuited_asc(r2.m_rank, r1.m_rank, is_suited);
                        }
                        else
                        {
                            add_suited_nonsuited_asc(r1.m_rank, r2.m_rank, is_suited);
                        }
                    }
                    else if (size == 3)
                    {
                        // 3rd pos should be +/s/o
                        if (tok[2] != '+' && tok[2] != 's' && (tok[2] != 'o'))
                        {
                            throw_invalid(tok);
                        }

                        // '+' can only be valid for pairs, i.e. "99+"
                        if (tok[2] == '+')
                        {
                            if (r1 != r2)
                            {
                                throw_invalid(tok);
                            }
                            add_pair_asc(r1.m_rank);
                            continue;
                        }

                        // must be a suited / nonsuited combo (no pair)
                        if (r1 == r2)
                        {
                            throw_invalid(tok);
                        }

                        // 4 cases: suited/nonsuited and correct/incorrect order
                        const bool is_suited = (tok[2] == 's');
                        if (r1 < r2)
                        {
                            if (is_suited)
                            {
                                set_value(hand_2r{r2, r1}, 400);
                            }
                            else
                            {
                                set_value(hand_2r{r1, r2}, 1200);
                            }
                        }
                        else    // (r2 < r1)
                        {
                            if (is_suited)
                            {
                                set_value(hand_2r{r1, r2}, 400);
                            }
                            else
                            {
                                set_value(hand_2r{r2, r1}, 1200);
                            }
                        }
                    }
                    else if (size == 2)
                    {
                        // must be a pair
                        if (r1 != r2)
                        {
                            throw_invalid(tok);
                        }
                        set_value(hand_2r{r1, r1}, 600);
                    }
                }
                catch (...)
                {
                    std::throw_with_nested(std::runtime_error("range from_string() encountered an exception while parsing token: " + tok));
                }
            }
        }

       public:
        ///////////////////////////////////////////////////////////////////////////////////////
        // CTORS
        ///////////////////////////////////////////////////////////////////////////////////////

        // empty range
        constexpr range() = default;

        // create with container of value_type hand_2r
        template <typename T,
                  std::enable_if_t<std::is_same_v<typename std::iterator_traits<typename T::iterator>::value_type, hand_2r>, int> = 0>
        constexpr explicit range(const T& ct) noexcept
        {
            // set each h2r to max
            for (const auto hor : ct)
            {
                m_combos[index(hor)] = max_value(hor);
            }
        }

        // create from string, comma separated ("99+,A2s+,KQo+,67s")
        // A5s+ adds A5s, A6s...AKs; KJo+ does the same as for non-suited
        // for pairs, 99+ will add 99, TT, JJ, QQ, KK, AA
        explicit range(const std::string_view sv) { from_string(sv); }

        // create from handmap
        //explicit range(const handmap& hm) noexcept { from_handmap(hm); }

        ///////////////////////////////////////////////////////////////////////////////////////
        // STATIC functions
        ///////////////////////////////////////////////////////////////////////////////////////

        // get the hand_2r for any (valid) index
        [[nodiscard]] constexpr static hand_2r hand(const uint8_t index)
        {
            if (index > c_rangeindex_max)
            {
                throw std::runtime_error("get_hand(const uint8_t) index out of bounds: " + std::to_string(index));
            }

            const uint8_t top = c_rank_ace - index % c_num_ranks;
            const uint8_t left = c_rank_ace - index / c_num_ranks;
            return hand_2r(left, top);
        }

        // get the index for hand_2r
        [[nodiscard]] constexpr static uint8_t index(const hand_2r h2r) noexcept
        {
            const auto [a, b] = h2r.as_pair();
            return static_cast<uint8_t>((c_rank_ace - a.m_rank) * c_num_ranks + (c_rank_ace - b.m_rank));
        }

        // get the index for hand_2c
        [[nodiscard]] constexpr static uint8_t index(const hand_2c h2c) noexcept
        {
            auto [a, b] = h2c.as_pair();

            if (a.rank() == b.rank())
            {
                return static_cast<uint8_t>((c_rank_ace - a.rank().m_rank) * c_num_ranks + (c_rank_ace - a.rank().m_rank));
            }
            else
            {
                if (a.suit() == b.suit())
                {
                    // suited -> (13 - hi.rank) * 13 + (13 - lo.rank)
                    return static_cast<uint8_t>((c_rank_ace - (a.rank() > b.rank() ? a.rank().m_rank : b.rank().m_rank)) * c_num_ranks +
                                                (c_rank_ace - (a.rank() > b.rank() ? b.rank().m_rank : a.rank().m_rank)));
                }
                else
                {
                    // non-suited -> lower rank comes first
                    return static_cast<uint8_t>((c_rank_ace - (a.rank() < b.rank() ? a.rank().m_rank : b.rank().m_rank)) * c_num_ranks +
                                                (c_rank_ace - (a.rank() < b.rank() ? b.rank().m_rank : a.rank().m_rank)));
                }
            }
        }

        // return max value for each index
        [[nodiscard]] constexpr static uint16_t max_value(const uint8_t index) noexcept
        {
            const uint8_t top = c_rank_ace - index % c_num_ranks;
            const uint8_t left = c_rank_ace - index / c_num_ranks;

            if (top == left)
            {
                // pair => factor 6
                return 100 * 6;
            }
            else
            {
                if (const bool is_suited = left > top; is_suited)
                {
                    // suited => factor 4
                    return 100 * 4;
                }
                else
                {
                    // must be non-suited
                    return 100 * 12;
                }
            }
        }

        // return max value for each hand_2r
        [[nodiscard]] constexpr static uint16_t max_value(const hand_2r h) { return max_value(index(h)); }

        ///////////////////////////////////////////////////////////////////////////////////////
        // ACCESSORS
        ///////////////////////////////////////////////////////////////////////////////////////

        // get the numer of "real hands" (entries x factor 4/6/12) with value > 0
        [[nodiscard]] constexpr uint16_t hands() const noexcept
        {
            uint16_t ret = 0;
            for (uint8_t i = 0; i < c_range_size; ++i)
            {
                const auto value = m_combos[i];
                if (value == 0)
                {
                    continue;
                }

                // get the factor for each entry and add to total
                const uint8_t top = c_rank_ace - i % c_num_ranks;
                const uint8_t left = c_rank_ace - i / c_num_ranks;
                if (top == left)
                {
                    // pair => factor 6
                    ret += 6;
                }
                else
                {
                    if (const bool is_suited = left > top; is_suited)
                    {
                        // suited => factor 4
                        ret += 4;
                    }
                    else
                    {
                        // must be non-suited => factor 12
                        ret += 12;
                    }
                }
            }
            return ret;
        }

        // return weighted sum which is actually encoded in the values
        // max value: (13*600+78*400+78*1200) = 132600
        [[nodiscard]] constexpr uint32_t total() const noexcept
        {
            //return static_cast<uint32_t>(std::reduce(m_combos.cbegin(), m_combos.cend(), 0));
            return std::reduce(m_combos.cbegin(), m_combos.cend(), uint32_t(0));
        }

        // percentage of full range
        [[nodiscard]] constexpr uint16_t percent() const noexcept
        {
            // 13 pairs (x6), the diagonal
            // 78 suited combinations (x4), upper triangle
            // 78 non-suited combinations (x12), lower triangle
            return static_cast<uint16_t>(total() * 100 / ((13 * 600) + (78 * 400) + (78 * 1200)));
        }

        // get the numer of entries (hand_2r objects) with value > 0 (not counting the factor 4/6/12)
        [[nodiscard]] constexpr uint8_t size() const noexcept
        {
            return std::accumulate(m_combos.cbegin(), m_combos.cend(), uint8_t(0),
                                   [](const uint8_t r, const uint16_t value) -> uint8_t { return value > 0 ? r + 1 : r; });

            /*
#if defined(__clang__) || !(defined(__GNUC__) || defined(_MSC_VER))
            // clang 11 does not support c++20 constexpr accumualte yet
            // also exclude other compilers, only gcc and msvc currently support it
            uint8_t ret = 0;
            for (uint8_t i = 0; i < c_range_size; i++)
            {
                if (const auto value = m_combos[i]; value > 0)
                {
                    ++ret;
                }
            }
            return ret;
#else
            // msvc 16.8 and gcc 10 support c++ 20 constexpr accumualte
            return std::accumulate(m_combos.cbegin(), m_combos.cend(), uint8_t(0),
                                   [](const uint8_t r, const uint16_t value) -> uint8_t { return value > 0 ? r + 1 : r; });
            // was: static_cast<uint8_t>(r + 1);
#endif
            */
        }

        // get value at index i
        [[nodiscard]] constexpr uint16_t value_of(const uint8_t index) const
        {
            if (index > c_rangeindex_max)
            {
                throw std::runtime_error("value_of() index out of bounds: " + std::to_string(index));
            }

            return m_combos[index];
        }

        // get value for hand_2r h
        [[nodiscard]] constexpr uint16_t value_of(const hand_2r h) const noexcept { return m_combos[index(h)]; }

        // get normalized value at index i
        [[nodiscard]] constexpr uint16_t normalized_value_of(const uint8_t index) const
        {
            if (index > c_rangeindex_max)
            {
                throw std::runtime_error("value_of() index out of bounds: " + std::to_string(index));
            }

            return m_combos[index] * 100 / max_value(index);
        }

        // get normalized value for hand_2r h
        [[nodiscard]] constexpr uint16_t normalized_value_of(const hand_2r h) const noexcept
        {
            return m_combos[index(h)] * 100 / max_value(index(h));
        }

        // print complete information about stored range
        [[nodiscard]] std::string str() const noexcept
        {
            std::string out("   | A | K | Q | J | T | 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 \n");
            for (uint8_t i = 0; i < c_range_size; i++)
            {
                const int8_t top = c_rank_ace - i % c_num_ranks;
                const int8_t left = c_rank_ace - i / c_num_ranks;
                const auto value = m_combos[i];

                if (top == c_rank_ace)
                {
                    out.append("---+---+---+---+---+---+---+---+---+---+---+---+---+---\n " + rank{rank_t(left)}.str() + " |");
                }

                if (top == left)
                {
                    //out.append(rank{rank_t(top)}.str() + rank{rank_t(top)}.str() + " :");
                    if (value > 0)
                    {
                        const auto tmp = std::to_string(value / 6);
                        out.append(std::string(" ", 3 - tmp.length()) + tmp);
                    }
                }
                else
                {
                    const bool is_suited = left > top;
                    //out.append(rank{rank_t(is_suited ? left : top)}.str() + rank{rank_t(is_suited ? top : left)}.str() +
                    //           (is_suited ? "s:" : "o:"));
                    if (value > 0)
                    {
                        const auto tmp = std::to_string(value / (is_suited ? 4 : 12));
                        out.append(std::string(" ", 3 - tmp.length()) + tmp);
                    }
                }

                if (value == 0)
                {
                    out.append("  0");
                }

                out.append(top == c_rank_two ? "\n" : "|");
            }
            return out;
        }

        /*
        // convert to handmap
        [[nodiscard]] handmap to_handmap() const noexcept
        {
            handmap out{};
            for (uint8_t i = 0; i < c_range_size; ++i)
            {
                const int8_t top = c_rank_ace - i % c_num_ranks;
                const int8_t left = c_rank_ace - i / c_num_ranks;
                const auto value = m_combos[i];

                if (value == 0)
                {
                    continue;
                }

                if (top == left)
                {
                    // we store 6 pairs in one abstraction => divide value by 6
                    out.add_pair(mkpoker::cards::rank{top}, static_cast<uint8_t>(value / 6));
                }
                else
                {
                    // suited or nonsuited, adjust value accordingly
                    if (const bool is_suited = left > top; is_suited)
                    {
                        out.add_suited(mkpoker::cards::rank{left}, mkpoker::cards::rank{top}, static_cast<uint8_t>(value / 4));
                    }
                    else
                    {
                        out.add_nonsuited(mkpoker::cards::rank{left}, mkpoker::cards::rank{top}, static_cast<uint8_t>(value / 12));
                    }
                }
            }
            return out;
        }

        // convert from handmap
        void from_handmap(const handmap& cm) noexcept
        {
            auto sum_for_hands = [&cm](const uint16_t ret, const hand& h) -> uint16_t { return ret + cm.value_of(h); };

            for (uint8_t i = 0; i < c_range_size; ++i)
            {
                const int8_t top = c_rank_ace - i % c_num_ranks;
                const int8_t left = c_rank_ace - i / c_num_ranks;

                if (top == left)
                {
                    // pair
                    const auto v_hands = mkpoker::util::generate_pairs(mkpoker::cards::rank{top});
                    const auto sum = std::accumulate(v_hands.cbegin(), v_hands.cend(), (uint16_t)0, sum_for_hands);
                    set_value(i, sum);
                }
                else
                {
                    // suited or nonsuited
                    if (const bool is_suited = left > top; is_suited)
                    {
                        const auto v_hands = mkpoker::util::generate_suited(mkpoker::cards::rank{left}, mkpoker::cards::rank{top});
                        const auto sum = std::accumulate(v_hands.cbegin(), v_hands.cend(), (uint16_t)0, sum_for_hands);
                        set_value(i, sum);
                    }
                    else
                    {
                        const auto v_hands = mkpoker::util::generate_nonsuited(mkpoker::cards::rank{left}, mkpoker::cards::rank{top});
                        const auto sum = std::accumulate(v_hands.cbegin(), v_hands.cend(), (uint16_t)0, sum_for_hands);
                        set_value(i, sum);
                    }
                }
            }
        }
        */

        ///////////////////////////////////////////////////////////////////////////////////////
        // MUTATORS
        ///////////////////////////////////////////////////////////////////////////////////////

        // set all values to zero
        void clear() { m_combos.fill(0); }

        // set all values to 100
        void fill() noexcept
        {
            for (uint8_t i = 0; i < c_range_size; i++)
            {
                m_combos[i] = max_value(i);
            }
        }

        // set value at index i, throws if invalid input
        void set_value(const uint8_t index, const uint16_t value)
        {
            if (index > c_rangeindex_max)
            {
                throw std::runtime_error("set_value(const uint8_t, const uint16_t): index out of bounds '" + std::to_string(index) + "'");
            }

            if (value > max_value(index))
            {
                throw std::runtime_error("set_value(const uint8_t, const uint16_t): value out of bounds '" + std::to_string(value) + "'");
            }

            m_combos[index] = value;
        }

        // set value at hand_2r
        void set_value(const hand_2r h2r, const uint16_t value)
        {
            const auto idx = index(h2r);

            if (value > max_value(idx))
            {
                throw std::runtime_error("set_value(const hand_2r): value out of bounds '" + std::to_string(value) + "'");
            }

            m_combos[idx] = value;
        }

        // set normalized value at index i, throws if invalid input
        void set_normalized_value(const uint8_t index, const uint8_t value)
        {
            if (index > c_rangeindex_max)
            {
                throw std::runtime_error("set_normalized_value(const uint8_t, const uint8_t): index out of bounds '" +
                                         std::to_string(index) + "'");
            }

            if (value > 100)
            {
                throw std::runtime_error("set_normalized_value(const uint8_t, const uint8_t): value out of bounds '" +
                                         std::to_string(value) + "'");
            }

            m_combos[index] = value * max_value(index) / 100;
        }

        ///////////////////////////////////////////////////////////////////////////////////////
        // helper functions
        ///////////////////////////////////////////////////////////////////////////////////////

        //
        // overloads for operator[]

        constexpr uint16_t operator[](const uint8_t ui) const { return value_of(ui); }
        constexpr uint16_t operator[](const hand_2r h2r) const noexcept { return value_of(h2r); }

        //
        // comparison operators

        constexpr bool operator==(const range&) const noexcept = default;
    };

}    // namespace mkp
