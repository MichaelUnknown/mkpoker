/*

TBD

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

#include <mkpoker/cfr/node.hpp>
#include <mkpoker/game/game.hpp>

#include <cstddef>
#include <cstdint>
#include <vector>

// enumerate gamestates
template <std::size_t N = 2>
struct gamestate_enumerator
{
    uint32_t counter = 0;
    std::vector<mkp::gamestate<N>> storage = {};

    uint32_t encode(const mkp::gamestate<N>& gamestate)
    {
        storage.push_back(gamestate);
        return counter++;
    }

    mkp::gamestate<N> decode(const uint32_t i) const { return storage.at(i); }
};

int main()
{
    mkp::gamestate<2> game_2p{4000};    // 4BB, i.e., very shallow game
    gamestate_enumerator enc_2p{};
    auto gametree_base_2p = mkp::init_tree(game_2p, &enc_2p);
    gametree_base_2p->print_node();    // ~2000 nodes

    mkp::gamestate<3> game_3p{4000};
    gamestate_enumerator<3> enc_3p{};
    auto gametree_base_3p = mkp::init_tree(game_3p, &enc_3p);
    gametree_base_3p->print_node();    // ~40k nodes
}