/*

mkpoker - demo putting hand/board combinations in different buckets

Copyright (C) Michael Kn�rzer

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

#include <mkpoker/base/cardset.hpp>
#include <mkpoker/base/hand.hpp>
#include <mkpoker/base/normalize.hpp>
#include <mkpoker/holdem/holdem_evaluation.hpp>
#include <mkpoker/util/card_generator.hpp>

#include <array>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <unordered_map>
#include <vector>

#include <fmt/core.h>
#include <fmt/format.h>

template <class T>
inline void boost_hash_combine(std::size_t& seed, const T& v)
{
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

// h: hand, b: board
struct hand_board_t
{
    mkp::cardset h;
    mkp::cardset b;
    hand_board_t(const mkp::cardset& h, const mkp::cardset& b) : h(h), b(b){};

    [[nodiscard]] std::string str() const noexcept { return fmt::format("{}/{}", h.str(), b.str()); }
    constexpr auto operator<=>(const hand_board_t&) const noexcept = default;
};

struct hand_board_hash
{
    std::size_t operator()(const hand_board_t& hb) const
    {
        std::size_t seed = 0;
        boost_hash_combine(seed, hb.h.as_bitset());
        boost_hash_combine(seed, hb.b.as_bitset());
        return seed;
    }
};

int main()
{
    //
    // compute unique hand/flop combinations
    //
    std::vector<hand_board_t> all_hand_flop_combos;
    const std::string fn_all_hand_flop_combos{"all_hf_combos.bin"};
    fmt::print("computing all unique hand/flop combinations...");
    for (unsigned i = 0; i < mkp::c_deck_size; ++i)
    {
        for (unsigned j = i + 1; j < mkp::c_deck_size; ++j)
        {
            const mkp::cardset hand_cs{mkp::make_bitset(i, j)};
            const mkp::hand_2c hand_2c{static_cast<uint8_t>(i), static_cast<uint8_t>(j)};

            for (unsigned k = 0; k < mkp::c_deck_size; ++k)
            {
                mkp::card c3{static_cast<uint8_t>(k)};
                if (hand_cs.contains(c3))
                {
                    continue;
                }
                for (unsigned l = k + 1; l < mkp::c_deck_size; ++l)
                {
                    mkp::card c4{static_cast<uint8_t>(l)};
                    if (hand_cs.contains(c4))
                    {
                        continue;
                    }
                    for (unsigned m = l + 1; m < mkp::c_deck_size; ++m)
                    {
                        mkp::card c5{static_cast<uint8_t>(m)};
                        if (hand_cs.contains(c5))
                        {
                            continue;
                        }

                        const mkp::cardset flop{c3, c4, c5};
                        const auto arr = suit_normalization_permutation(hand_2c, flop);

                        all_hand_flop_combos.emplace_back(hand_cs.rotate_suits(arr), flop.rotate_suits(arr));
                    }
                }
            }
        }
    }
    std::sort(all_hand_flop_combos.begin(), all_hand_flop_combos.end());
    const auto it = std::unique(all_hand_flop_combos.begin(), all_hand_flop_combos.end());
    all_hand_flop_combos.erase(it, all_hand_flop_combos.end());
    //    all hand / flop combos : 25'989'600
    // unique hand / flop combos :  1'286'792
    fmt::print(" done!\n");

    //
    // make some (not very sophisticated) buckets
    //
    std::vector<std::string> buckets{"very nutted hands", "mediocre hands", "bad hands"};
    std::unordered_map<hand_board_t, int, hand_board_hash> lookup;
    std::vector<int> mapping(all_hand_flop_combos.size());
    fmt::print("bucketing all hand/flop combinations...");
    for (auto i = 0; i < all_hand_flop_combos.size(); ++i)
    {
        const auto elem = all_hand_flop_combos[i];
        const auto all_cards = elem.h.combine(elem.b);
        const auto eval = mkp::evaluate_safe(all_cards);
        if (eval.type() >= mkp::c_straight)
        {
            // nutted hands
            lookup.insert({elem, i});
            mapping[i] = 0;
        }
        else if (eval.type() == mkp::c_no_pair)
        {
            // air
            lookup.insert({elem, i});
            mapping[i] = 2;
        }
        else
        {
            // pairs, trips
            lookup.insert({elem, i});
            mapping[i] = 1;
        }
    }
    fmt::print(" done!\n");
    std::locale::global(std::locale("en_US.UTF-8"));
    const auto nutted = std::count(mapping.cbegin(), mapping.cend(), 0);
    const auto mediocre = std::count(mapping.cbegin(), mapping.cend(), 1);
    const auto bad = std::count(mapping.cbegin(), mapping.cend(), 2);
    fmt::print("distribution (nutted/mediocre/bad): {:L}/{:L}/{:L}", nutted, mediocre, bad);
    fmt::print("\n\n");

    //
    // take some samples
    //
    mkp::card_generator cgen{std::random_device{}()};
    for (auto i = 0; i < 10; ++i)
    {
        const auto random_cards = cgen.generate_v(5);
        const auto hand_2c = mkp::hand_2c(random_cards[0], random_cards[1]);
        const auto flop = mkp::cardset({random_cards[2], random_cards[3], random_cards[4]});
        const auto arr = suit_normalization_permutation(hand_2c, flop);
        const auto normalized_hand = hand_board_t(hand_2c.as_cardset().rotate_suits(arr), flop.rotate_suits(arr));

        const auto id = lookup.at(normalized_hand);
        const auto hb = all_hand_flop_combos[id];
        const auto bucket = mapping[id];
        const auto raw = fmt::format("{}/{}", hand_2c.str(), flop.str());
        fmt::print("hand/flop combo {}, normalized to {}, landed in bucket {} ({})\n", raw, hb.str(), bucket, buckets[bucket]);
    }

    return EXIT_SUCCESS;
}