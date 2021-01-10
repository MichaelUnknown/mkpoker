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
#include <mutex>
#include <thread>    // sleep 1s
#include <vector>

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

    /*
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
    */

    {
        // simplified game: only limit preflop actions are allowed
        // thus, we can use a 'range' abstraction that utilizes suit isomorphism
        mkp::action_abstraction_simple_preflop<2> aa_2p_fcr{};
        mkp::card_abstraction_by_range<2, uint32_t> ca_2p{};

        std::cout << "game with 2 players, stack size 100BB, simple_preflop filter\n";
        mkp::gamestate<2> game_2p{100'000};
        mkp::gamestate_enumerator<2, uint32_t> enc_2p{};
        auto gametree_base_2p = mkp::init_tree(game_2p, &enc_2p, &aa_2p_fcr);
        //gametree_base_2p->print_node();
        const auto sz_tree = tree_size(gametree_base_2p.get());
        const auto cnt_nodes = sz_tree.info + sz_tree.term;
        const auto new_mem =
            sz_tree.info * (sizeof(mkp::node_infoset<2, uint32_t>) + sizeof(std::unique_ptr<mkp::node_base<2, uint32_t>>)) +
            sz_tree.term * (sizeof(mkp::node_terminal<2, uint32_t>) + sizeof(std::unique_ptr<mkp::node_base<2, uint32_t>>));
        std::cout << "number of nodes: " << cnt_nodes << "\n";
        std::cout << "estimated size for tree nodes: " << new_mem << " (" << new_mem / 1024 << "KB)\n\n";

        mkp::cfr_data<2, uint32_t> cfrd_2p(std::move(gametree_base_2p), &enc_2p, &aa_2p_fcr, &ca_2p);

        std::vector<std::thread> workers;
        std::mutex mu;
        for (int tid = 0; tid < 8; ++tid)
        {
            workers.push_back(std::thread([&cfrd_2p, &mu, tid]() {
                mkp::card_generator cgen{};
                std::array<std::array<int32_t, 2>, 1024> util{};
                for (uint32_t i = 0; i < 50'000; ++i)
                {
                    if (i % 10'000 == 0)
                    {
                        const auto stats = mkp::regret_stats(cfrd_2p.m_root.get(), cfrd_2p.m_regret_sum);
                        const auto sum_util =
                            std::accumulate(util.cbegin(), util.cend(), int64_t(0),
                                            [](const int64_t lhs, const std::array<int32_t, 2>& rhs) { return lhs + rhs[0]; });
                        const auto sum_util_r =
                            std::accumulate(util.cbegin(), util.cend(), int64_t(0),
                                            [](const int64_t lhs, const std::array<int32_t, 2>& rhs) { return lhs + rhs[1]; });
                        const int64_t sz = util.size();

                        {
                            std::lock_guard<std::mutex> guard(mu);
                            std::cout << "thread: " << tid << "\n"
                                      << "stats after " << i << " iterations | sum: " << stats.sum << ", min: " << stats.min
                                      << ", max: " << stats.max << "\n average utility: " << sum_util / sz << "/" << sum_util_r / sz
                                      << std::endl;
                        }
                    }

                    const mkp::gamecards<2> cards(cgen.generate_v(9));
                    util[i % util.size()] = cfr_2p(cards, cfrd_2p, cfrd_2p.m_root.get(), {1.0, 1.0});
                }
            }));
        }
        std::for_each(workers.begin(), workers.end(), [](std::thread& t) { t.join(); });

        //cfrd_2p.print_strategy(cfrd_2p.m_root.get(), 0, 0);

        return EXIT_SUCCESS;
    }
}