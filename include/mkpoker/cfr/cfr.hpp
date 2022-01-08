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

#include <mkpoker/base/range.hpp>
#include <mkpoker/cfr/action_abstraction.hpp>
#include <mkpoker/cfr/card_abstraction.hpp>
#include <mkpoker/cfr/game_abstraction.hpp>
#include <mkpoker/cfr/node.hpp>
#include <mkpoker/game/game.hpp>
#include <mkpoker/util/algorithm.hpp>
#include <mkpoker/util/mtp.hpp>

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>
#include <memory>
#include <numeric>
#include <vector>

namespace mkp
{
    namespace detail
    {
        struct tree_size_t
        {
            std::size_t info = 0;
            std::size_t term = 0;

            tree_size_t& operator+=(const tree_size_t& other)
            {
                info += other.info;
                term += other.term;
                return *this;
            }
        };

        template <std::size_t N, typename T, UnsignedIntegral U>
        tree_size_t tree_size_impl(mkp::node_base<N, T, U>* ptr_root)
        {
            tree_size_t res{};
            for (auto&& child : ptr_root->m_children)
            {
                res += tree_size_impl(child.get());
            }

            if (ptr_root->is_terminal())
            {
                ++res.term;
            }
            else
            {
                ++res.info;
            }

            return res;
        }

        struct regret_stats_t
        {
            int64_t sum = 0;
            int32_t min = std::numeric_limits<int32_t>::max();
            int32_t max = std::numeric_limits<int32_t>::lowest();

            regret_stats_t& operator+=(const regret_stats_t& other)
            {
                sum += other.sum;
                min = other.min < min ? other.min : min;
                max = other.max > max ? other.max : max;
                return *this;
            }
        };

        template <std::size_t N, typename T, UnsignedIntegral U>
        regret_stats_t regret_stats_impl(mkp::node_base<N, T, U>* ptr_root, const std::vector<std::vector<std::vector<int32_t>>>& data)
        {
            regret_stats_t res{};
            if (ptr_root->is_terminal())
            {
                return res;
            }

            for (auto&& child : ptr_root->m_children)
            {
                res += regret_stats_impl(child.get(), data);
            }

            // we want to sum all the entries of one gamestate, so...
            // for each card abstraction id 'i', sum the values of all actions
            const auto gs_id = ptr_root->m_id;
            for (uint32_t i = 0; i < data[gs_id].size(); ++i)
            {
                const auto local_sum = std::reduce(data[gs_id][i].cbegin(), data[gs_id][i].cend());
                const auto [local_min, local_max] = std::minmax_element(data[gs_id][i].cbegin(), data[gs_id][i].cend());
                res.sum += local_sum;
                res.min = *local_min < res.min ? *local_min : res.min;
                res.max = *local_max > res.max ? *local_max : res.max;
            }

            return res;
        }
    }    // namespace detail

    // computes the number of nodes
    template <std::size_t N, typename T, UnsignedIntegral U>
    auto tree_size(mkp::node_base<N, T, U>* ptr_root)
    {
        return detail::tree_size_impl(ptr_root);
    }

    // computes the sum of all regret / strategy entries
    template <std::size_t N, typename T, UnsignedIntegral U>
    auto regret_stats(mkp::node_base<N, T, U>* ptr_root, const std::vector<std::vector<std::vector<int32_t>>>& data)
    {
        return detail::regret_stats_impl(ptr_root, data);
    }

    template <std::size_t N, typename T, UnsignedIntegral U = uint32_t>
    struct cfr_data
    {
        // 3 layers:
        // - gamestate
        //  - cards/card_abstraction_id
        //   - action
        //
        // since we traverse the game tree with fixed cards, this layout (vector of actions for each card abstraction id)
        // should be more cache friendly than the other way round, although it makes printint the tree a little bit
        // more cumbersome

        std::vector<std::vector<std::vector<int32_t>>> m_regret_sum;
        std::vector<std::vector<std::vector<int32_t>>> m_strategy_sum;
        std::unique_ptr<node_base<N, T, U>> m_root;
        const game_abstraction_base<T, U>* m_ptr_ga;
        const action_abstraction_base<T>* m_ptr_aa;
        const card_abstraction_base<N, U>* m_ptr_ca;

        cfr_data() = delete;
        cfr_data(std::unique_ptr<node_base<N, T, U>> root, game_abstraction_base<T, U>* ptr_ga, action_abstraction_base<T>* ptr_aa,
                 card_abstraction_base<N, U>* ptr_ca)
            : m_root(std::move(root)), m_ptr_ga(ptr_ga), m_ptr_aa(ptr_aa), m_ptr_ca(ptr_ca)
        {
            init(m_root.get());
        }

        // print nodes recursively
        void print_strategy(const node_base<N, T, U>* ptr_node, const int level, const int print_level_max = 128) const
        {
            if (level > print_level_max)
            {
                return;
            }

            const auto space = std::string(level, ' ');

            const auto gs_id = ptr_node->m_id;
            const auto gs = m_ptr_ga->decode(gs_id);
            const auto all_actions = m_ptr_aa->filter_actions(gs);

            std::cout << space << gs.str_state() << "\n";

            // in case of preflop ranges, use pretty print
            if (ptr_node->m_game_state == gb_gamestate_t::PREFLOP_BET && m_strategy_sum[gs_id].size() == c_range_size)
            {
                std::vector<range> vec_ranges(all_actions.size());
                for (uint8_t i = 0; i < c_range_size; ++i)
                {
                    const auto values = normalize(m_strategy_sum[gs_id][i]);
                    for (uint32_t j = 0; j < vec_ranges.size(); ++j)
                    {
                        vec_ranges[j].set_normalized_value(i, static_cast<uint8_t>(values[j] * 100));
                    }
                }

                for (uint8_t i = 0; i < vec_ranges.size(); ++i)
                {
                    std::cout << space << (all_actions[i].str()) << ": " << std::fixed << std::setprecision(2)
                              << std::to_string(vec_ranges[i].percent()) << "%\n";

                    std::cout << vec_ranges[i].str();
                    std::cout << "\n";
                }
            }
            else
            {
                const std::vector<std::pair<uint32_t, float>> vec_init;
                std::vector<std::vector<std::pair<uint32_t, float>>> vec_temp(all_actions.size(), vec_init);
                for (uint32_t i = 0; i < m_strategy_sum[gs_id].size(); ++i)
                {
                    // skip empty indices
                    if (std::reduce(m_strategy_sum[gs_id][i].cbegin(), m_strategy_sum[gs_id][i].cend()) == 0)
                    {
                        continue;
                    }

                    const auto values = normalize(m_strategy_sum[gs_id][i]);
                    for (uint32_t j = 0; j < values.size(); ++j)
                    {
                        vec_temp[j].emplace_back(i, values[j]);
                    }
                }

                for (uint32_t i = 0; i < vec_temp.size(); ++i)
                {
                    std::cout << space << (all_actions[i].str()) << ": " << std::fixed << std::setprecision(2)
                              << std::accumulate(vec_temp[i].cbegin(), vec_temp[i].cend(), 0.0f,
                                                 [](const float val, const auto& e) { return val + e.second; }) *
                                     100 / vec_temp[i].size()
                              << "%\n";

                    std::sort(vec_temp[i].begin(), vec_temp[i].end(),
                              [](const auto& lhs, const auto& rhs) { return lhs.second > rhs.second; });
                    for (uint32_t j = 0; j < vec_temp[i].size(); ++j)
                    {
                        if (j < 25 || j > vec_temp[i].size() - 5)
                            std::cout << space << m_ptr_ca->str_id(ptr_node->m_game_state, vec_temp[i][j].first) << " => "
                                      << vec_temp[i][j].second << "\n";
                    }
                    std::cout << "\n";
                }
                std::cout << "\n";
            }

            for (auto&& e : ptr_node->m_children)
            {
                if (!e->is_terminal())
                {
                    print_strategy(e.get(), level + 1, print_level_max);
                }
            }
        }

       private:
        void init(node_base<N, T, U>* ptr_node)
        {
            // size of inner vector: number of actions / children of that node
            const auto inner = std::vector<int32_t>(ptr_node->m_children.size(), 0);
            // size of outer vector: numer of possible card combinations according to card abstraction
            const auto init_vec = std::vector<std::vector<int32_t>>(m_ptr_ca->size(ptr_node->m_game_state), inner);
            m_regret_sum.push_back(init_vec);
            m_strategy_sum.push_back(init_vec);

            for (auto&& child : ptr_node->m_children)
            {
                init(child.get());
            }
        }
    };

    // each int value in the vector corresponds to an action, encoded as the position inside the vector
    // the value is the reward of that action, e.g., preflop raising with aces might have a high
    // positive value, raising with 72o a negative value
    //
    // computes the best strategy from regret sum, disregards actions with negative regrets
    std::vector<float> get_strategy(const std::vector<int32_t>& regrets)
    {
        std::vector<int32_t> positive_regrets;
        positive_regrets.reserve(regrets.size());
        std::transform(regrets.cbegin(), regrets.cend(), std::back_inserter(positive_regrets),
                       [](const int32_t i) -> int32_t { return i < 0 ? 0 : i; });
        return normalize(positive_regrets);
    }

    // return the averaged strategy after training
    std::vector<float> average_strategy(const std::vector<int32_t>& strategy) { return normalize(strategy); }

    // update the strategy sum
    void update_strategy_sum(std::vector<int32_t>& strategy_sum, const std::vector<float>& new_strategy, const float p)
    {
        for (std::size_t i = 0; i < strategy_sum.size(); ++i)
            strategy_sum[i] += static_cast<int32_t>(p * 100.0f * new_strategy[i]);
    }

    // recursively compute the utilization for the current node, given a set of cards
    // uses cfrd to store regrets / strategy
    template <typename game_type>
    std::array<int32_t, 2> cfr_2p(const gamecards<2>& cards, cfr_data<2, game_type, uint32_t>& cfrd,
                                  node_base<2, game_type, uint32_t>* ptr_node, std::array<float, 2> reach)
    {
        // if the node is terminal, return utility
        if (ptr_node->is_terminal())
        {
            return ptr_node->utility(cards, cfrd.m_ptr_ga);
        }

        // otherwise, call cfr for each action recursively with updated reach for the active player

        const auto ap = ptr_node->m_active_player;
        const auto game_abstraction_id = ptr_node->m_id;
        const auto card_abstraction_id = cfrd.m_ptr_ca->id(ptr_node->m_game_state, ptr_node->m_active_player, cards);

        // get new strategy, update strategy sum
        const auto strategy = get_strategy(cfrd.m_regret_sum[game_abstraction_id][card_abstraction_id]);
        update_strategy_sum(cfrd.m_strategy_sum[game_abstraction_id][card_abstraction_id], strategy, reach[ap]);

        const auto& all_nodes = ptr_node->m_children;
        std::array<int32_t, 2> node_utility{0, 0};
        std::vector<std::array<int32_t, 2>> utility_all_children;
        utility_all_children.reserve(all_nodes.size());
        for (std::size_t i = 0; i < all_nodes.size(); ++i)
        {
            auto reach_new = reach;
            reach_new[ap] *= strategy[i];
            const auto utility_this_child = cfr_2p(cards, cfrd, all_nodes[i].get(), reach_new);
            utility_all_children.push_back(utility_this_child);
            const auto adjusted_utility_this_child = utility_this_child * strategy[i];
            node_utility += adjusted_utility_this_child;
        }

        // update regrets
        for (std::size_t i = 0; i < all_nodes.size(); ++i)
        {
            const auto regret_active_player = utility_all_children[i][ap] - node_utility[ap];
            cfrd.m_regret_sum[game_abstraction_id][card_abstraction_id][i] += static_cast<int32_t>(reach[1 - ap] * regret_active_player);
        }

        return node_utility;
    }

}    // namespace mkp
