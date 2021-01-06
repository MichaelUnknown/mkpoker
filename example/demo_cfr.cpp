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

#include <mkpoker/cfr/action_abstraction.hpp>
#include <mkpoker/cfr/card_abstraction.hpp>
#include <mkpoker/cfr/cfr.hpp>
#include <mkpoker/cfr/game_abstraction.hpp>
#include <mkpoker/cfr/node.hpp>
#include <mkpoker/game/game.hpp>
#include <mkpoker/util/card_generator.hpp>

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
    return 1 + res;
}

int main()
{
    //{
    //    std::cout << "game with 2 players, stack size 12BB, no action filter\n";
    //    mkp::gamestate<2> game_2p{12000};
    //    mkp::gamestate_discarder<2, uint32_t> enc_2p{};
    //    mkp::action_abstraction_noop<2> aa_2p{};
    //    auto gametree_base_2p = mkp::init_tree(game_2p, &enc_2p, &aa_2p);
    //    std::cout << "init done" << std::endl;
    //    const auto cnt_nodes = tree_size(gametree_base_2p.get());
    //    const auto sz_node = sizeof(decltype(gametree_base_2p)) + sizeof(decltype(*gametree_base_2p));
    //    const auto mem = cnt_nodes * sz_node;
    //    std::cout << "number of nodes: " << cnt_nodes << "\n";
    //    std::cout << "estimated size: " << mem << " (" << mem / 1024 / 1024 << "MB)\n\n";
    //    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    //    return 0;
    //}

    {
        mkp::gamestate<2> game_2p{4'000};    // 4BB, i.e., very shallow game
        mkp::gamestate_enumerator<2, uint32_t> enc_2p{};
        mkp::action_abstraction_noop<2> aa_2p{};
        auto gametree_base_2p = mkp::init_tree(game_2p, &enc_2p, &aa_2p);
        //gametree_base_2p->print_node();    // ~2000 nodes
        std::cout << "game with 2 players, stack size 4BB, no action filter\n"
                  << "number of nodes: " << tree_size(gametree_base_2p.get()) << "\n\n";
    }
    {
        mkp::gamestate<3> game_3p{4'000};
        mkp::gamestate_enumerator<3, uint32_t> enc_3p{};
        mkp::action_abstraction_noop<3> aa_3p{};
        auto gametree_base_3p = mkp::init_tree(game_3p, &enc_3p, &aa_3p);
        //gametree_base_3p->print_node();    // ~40k nodes
        std::cout << "game with 3 players, stack size 4BB, no action filter\n"
                  << "number of nodes: " << tree_size(gametree_base_3p.get()) << "\n\n";
    }
    {
        mkp::gamestate<6> game_6p{2'000};
        mkp::gamestate_enumerator<6, uint32_t> enc_6p{};
        mkp::action_abstraction_noop<6> aa_6p{};
        auto gametree_base_6p = mkp::init_tree(game_6p, &enc_6p, &aa_6p);
        //gametree_base_6p->print_node();    // ~100k nodes
        std::cout << "game with 6 players, stack size 2BB, no action filter\n"
                  << "number of nodes: " << tree_size(gametree_base_6p.get()) << "\n\n";
    }
    {
        mkp::gamestate<6> game_6p{10'000};
        mkp::gamestate_enumerator<6, uint32_t> enc_6p{};
        mkp::action_abstraction_fcr<6> aa_6p_fcr{};    // fcr will filter out most actions
        auto gametree_base_6pn = mkp::init_tree(game_6p, &enc_6p, &aa_6p_fcr);
        //gametree_base_6pn->print_node();    // ~25k nodes
        std::cout << "game with 6 players, stack size 10BB, allow only fold, call, raise (allin)\n"
                  << "number of nodes: " << tree_size(gametree_base_6pn.get()) << "\n\n";
    }

    {
        std::cout << "game with 2 players, stack size 100BB, fcr filter\n";
        mkp::gamestate<2> game_2p{4'000};
        mkp::gamestate_enumerator<2, uint32_t> enc_2p{};
        mkp::action_abstraction_fcr<2> aa_2p_fcr{};
        auto gametree_base_2p = mkp::init_tree(game_2p, &enc_2p, &aa_2p_fcr);
        std::cout << "init done" << std::endl;
        const auto cnt_nodes = tree_size(gametree_base_2p.get());
        const auto sz_node = sizeof(decltype(gametree_base_2p)) + sizeof(decltype(*gametree_base_2p));
        const auto mem = cnt_nodes * sz_node;
        std::cout << "number of nodes: " << cnt_nodes << "\n";
        std::cout << "estimated size: " << mem << " (" << mem / 1024 << "KB)\n\n";

        mkp::card_abstraction_by_range<2, uint32_t> ca_2p{};
        mkp::cfr_data<2, uint32_t> cfrd_2p(std::move(gametree_base_2p), &enc_2p, &aa_2p_fcr, &ca_2p);

        mkp::card_generator cgen{};
        std::array<int32_t, 2> util;
        for (int i = 0; i < 100'000; ++i)
        {
            const auto random_cards = cgen.generate_v(9);
            const mkp::gamecards<2> cards(random_cards);
            util = cfr_2p(cards, cfrd_2p, cfrd_2p.m_root.get(), {1.0, 1.0});
        }

        cfrd_2p.print_strategy_r(cfrd_2p.m_root.get(), 0, 1);
    }
}