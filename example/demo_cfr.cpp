/*

mkpoker - demo app that trains an 'AI' with the CFR algorithm

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

#include <mkpoker/cfr/action_abstraction.hpp>
#include <mkpoker/cfr/card_abstraction.hpp>
#include <mkpoker/cfr/cfr.hpp>
#include <mkpoker/cfr/game_abstraction.hpp>
#include <mkpoker/cfr/node.hpp>
#include <mkpoker/game/game.hpp>
#include <mkpoker/util/card_generator.hpp>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

int main()
{
    // if you do not restrict the number of actions for each street ('action abstraction'), the number of
    // nodes gets huge really fast... some examples

    using game_type = mkp::gamestate<2, 0, 1>;

    {
        // 4BB, i.e., very very shallow game, no action abstraction
        // results in ~4000 nodes
        mkp::gamestate<2, 0, 1> game_2p{4'000};
        mkp::gamestate_enumerator<mkp::gamestate<2, 0, 1>, uint32_t> enc_2p{};
        mkp::action_abstraction_noop<mkp::gamestate<2, 0, 1>> aa_2p{};
        auto gametree_base_2p = mkp::init_tree(game_2p, &enc_2p, &aa_2p);

        const auto [i, t] = tree_size(gametree_base_2p.get());
        const auto cnt_nodes = i + t;
        std::cout << "game with 2 players, stack size 4BB, no action filter\n"
                  << "number of info nodes (info/terminal/all): " << i << "/" << t << "/" << cnt_nodes << "\n\n";
        //gametree_base_2p->print_node();
    }

    {
        // 10BB, i.e., still very shallow game, no action abstraction
        // results in ~2.5m nodes
        game_type game_2p{10'000};
        mkp::gamestate_enumerator<game_type, uint32_t> enc_2p{};
        mkp::action_abstraction_noop<game_type> aa_2p{};

        auto gametree_base_2p = mkp::init_tree(game_2p, &enc_2p, &aa_2p);
        const auto [i, t] = tree_size(gametree_base_2p.get());
        const auto cnt_nodes = i + t;
        std::cout << "game with 2 players, stack size 10BB, no action filter\n"
                  << "number of info nodes (info/terminal/all): " << i << "/" << t << "/" << cnt_nodes << "\n\n";
    }
    {
        // simplified game: only a couple preflop actions are allowed (fold, call, raise with specific sizes)
        // after the flop, the game is checked down to the river / end of the hand (i.e., we play 'preflop poker')
        // 200BB stack depth for 2 players results in just 113 nodes
        game_type game_2p{200'000};
        mkp::gamestate_enumerator<game_type, uint32_t> enc_2p{};
        mkp::action_abstraction_simple_preflop<game_type> aa_2p_fcr{};
        auto gametree_base_2p = mkp::init_tree(game_2p, &enc_2p, &aa_2p_fcr);
        const auto [i, t] = tree_size(gametree_base_2p.get());
        const auto cnt_nodes = i + t;
        std::cout << "game with 2 players, stack size 200BB, action filter 'preflop poker'\n"
                  << "number of info nodes (info/terminal/all): " << i << "/" << t << "/" << cnt_nodes << "\n";
        std::cout << "\n------------------------------------------------------\n\n";
    }

    // with the 'preflop' game, we can train an AI using the CFR algorithm
    // we use a 'range' card_abstraction that utilizes suit isomorphism to reduce the
    // number of entries needed in the cfr data from 52*51=2652 to 13*13=169
    {
        game_type game_2p{200'000};
        mkp::gamestate_enumerator<game_type, uint32_t> enc_2p{};
        mkp::action_abstraction_simple_preflop<game_type> aa_2p_fcr{};
        mkp::card_abstraction_by_range<2, uint32_t> ca_2p{};

        auto gametree_base_2p = mkp::init_tree(game_2p, &enc_2p, &aa_2p_fcr);
        mkp::cfr_data<2, game_type, uint32_t> cfrd_2p(std::move(gametree_base_2p), &enc_2p, &aa_2p_fcr, &ca_2p);

        std::vector<std::thread> workers;
        std::mutex mu;
        for (int tid = 0; tid < 8; ++tid)
        {
            workers.push_back(std::thread([&cfrd_2p, &mu, tid]() {
                mkp::card_generator cgen{};
                std::array<std::array<int32_t, 2>, 65536> util{};
                for (uint32_t i = 0; i < 500'000; ++i)
                {
                    if (i % 50'000 == 0)
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
                                      << ", max: " << stats.max << "\naverage utility: " << sum_util / sz << "/" << sum_util_r / sz
                                      << std::endl;
                        }
                    }

                    const mkp::gamecards<2> cards(cgen.generate_v(9));
                    util[i % util.size()] = cfr_2p(cards, cfrd_2p, cfrd_2p.m_root.get(), {1.0, 1.0});
                }
            }));
        }
        std::cout << "trainig 'preflop poker'...\n";
        std::for_each(workers.begin(), workers.end(), [](std::thread& t) { t.join(); });
        std::cout << "\n\n";

        // print the first two levels of the tree with action probabilities
        cfrd_2p.print_strategy(cfrd_2p.m_root.get(), 0, 1);
    }

    {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(100ms);
    }

    // same with rake
    {
        // use an unrealistic high amount of rake (20%), to show the difference
        // in the resulting strategy
        using game_type_w_r = mkp::gamestate<2, 200, 1'000>;

        game_type_w_r game_2p{200'000};
        mkp::gamestate_enumerator<game_type_w_r, uint32_t> enc_2p{};
        mkp::action_abstraction_simple_preflop<game_type_w_r> aa_2p_fcr{};
        mkp::card_abstraction_by_range<2, uint32_t> ca_2p{};

        auto gametree_base_2p = mkp::init_tree(game_2p, &enc_2p, &aa_2p_fcr);
        mkp::cfr_data<2, game_type_w_r, uint32_t> cfrd_2p(std::move(gametree_base_2p), &enc_2p, &aa_2p_fcr, &ca_2p);

        std::vector<std::thread> workers;
        std::mutex mu;
        for (int tid = 0; tid < 8; ++tid)
        {
            workers.push_back(std::thread([&cfrd_2p, &mu, tid]() {
                mkp::card_generator cgen{};
                std::array<std::array<int32_t, 2>, 65536> util{};
                for (uint32_t i = 0; i < 500'000; ++i)
                {
                    if (i % 50'000 == 0)
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
                                      << ", max: " << stats.max << "\naverage utility: " << sum_util / sz << "/" << sum_util_r / sz
                                      << std::endl;
                        }
                    }

                    const mkp::gamecards<2> cards(cgen.generate_v(9));
                    util[i % util.size()] = cfr_2p(cards, cfrd_2p, cfrd_2p.m_root.get(), {1.0, 1.0});
                }
            }));
        }
        std::cout << "trainig 'preflop poker'...\n";
        std::for_each(workers.begin(), workers.end(), [](std::thread& t) { t.join(); });
        std::cout << "\n\n";

        // print the first two levels of the tree with action probabilities
        cfrd_2p.print_strategy(cfrd_2p.m_root.get(), 0, 1);
    }

    return EXIT_SUCCESS;
}