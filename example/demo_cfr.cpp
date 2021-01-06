/*

TBD

Copyright (C) 2020 Michael Kn�rzer

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

#include <mkpoker/cfr/action_abstraction.hpp>
#include <mkpoker/cfr/game_abstraction.hpp>
#include <mkpoker/cfr/node.hpp>
#include <mkpoker/game/game.hpp>

#include <chrono>    // sleep 1s
#include <cstddef>
#include <cstdint>
#include <memory>
#include <thread>    // sleep 1s
#include <vector>

template <std::size_t N, typename T = uint32_t>
std::size_t tree_size(mkp::node_base<N, T>* game)
{
    std::size_t res{};
    for (auto&& child : game->m_children)
    {
        res += tree_size(child.get());
    }
    return res + game->m_children.size();
}

int main()
{
    {
        std::cout << "game with 2 players, stack size 25BB, no action filter\n";
        mkp::gamestate<2> game_2p{7000};
        mkp::gamestate_discarder<2, uint32_t> enc_2p{};
        mkp::action_abstraction_noop<2> aa_2p{};
        auto gametree_base_2p = mkp::init_tree(game_2p, &enc_2p, &aa_2p);
        std::cout << "init done" << std::endl;
        std::cout << "number of nodes: " << tree_size(gametree_base_2p.get()) << "\n";
        std::cout << "estimated size: " << tree_size(gametree_base_2p.get()) * sizeof(decltype(gametree_base_2p)) << " ("
                  << tree_size(gametree_base_2p.get()) * sizeof(decltype(gametree_base_2p)) / 1024 << "KB)\n\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(5000));
        return 0;
    }

    {
        mkp::gamestate<2> game_2p{4000};    // 4BB, i.e., very shallow game
        mkp::gamestate_enumerator<2, uint32_t> enc_2p{};
        mkp::action_abstraction_noop<2> aa_2p{};
        auto gametree_base_2p = mkp::init_tree(game_2p, &enc_2p, &aa_2p);
        //gametree_base_2p->print_node();    // ~2000 nodes
        std::cout << "\ngame with 2 players, stack size 4BB, no action filter\n"
                  << "number of nodes: " << tree_size(gametree_base_2p.get()) << "\n";
    }
    {
        mkp::gamestate<3> game_3p{4000};
        mkp::gamestate_enumerator<3, uint32_t> enc_3p{};
        mkp::action_abstraction_noop<3> aa_3p{};
        auto gametree_base_3p = mkp::init_tree(game_3p, &enc_3p, &aa_3p);
        //gametree_base_3p->print_node();    // ~40k nodes
        std::cout << "\ngame with 3 players, stack size 4BB, no action filter\n"
                  << "number of nodes: " << tree_size(gametree_base_3p.get()) << "\n";
    }
    {
        mkp::gamestate<6> game_6p{2000};
        mkp::gamestate_enumerator<6, uint32_t> enc_6p{};
        mkp::action_abstraction_noop<6> aa_6p{};
        auto gametree_base_6p = mkp::init_tree(game_6p, &enc_6p, &aa_6p);
        //gametree_base_6p->print_node();    // ~100k nodes
        std::cout << "\ngame with 6 players, stack size 2BB, no action filter\n"
                  << "number of nodes: " << tree_size(gametree_base_6p.get()) << "\n";
    }
    {
        mkp::gamestate<6> game_6p{10000};
        mkp::gamestate_enumerator<6, uint32_t> enc_6p{};
        mkp::action_abstraction_fcr<6> aa_6p_fcr{};    // fcr will filter out most actions
        auto gametree_base_6pn = mkp::init_tree(game_6p, &enc_6p, &aa_6p_fcr);
        //gametree_base_6pn->print_node();    // ~25k nodes
        std::cout << "\ngame with 6 players, stack size 10BB, allow only fold, call, raise (allin)\n"
                  << "number of nodes: " << tree_size(gametree_base_6pn.get()) << "\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    }
}