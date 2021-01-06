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

#include <mkpoker/cfr/action_abstraction.hpp>
#include <mkpoker/cfr/card_abstraction.hpp>
#include <mkpoker/cfr/game_abstraction.hpp>
#include <mkpoker/cfr/node.hpp>
#include <mkpoker/game/game.hpp>
#include <mkpoker/util/algorithm.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <memory>
#include <numeric>
#include <vector>

namespace mkp
{
    template <std::size_t N, typename T = uint32_t>
    struct cfr_data
    {
        using game_encode_type = T;

        // 3 layers:
        // - gamestate
        //  - cards/card_abstraction_id
        //   - action
        std::vector<std::vector<std::vector<int32_t>>> m_regret_sum;
        std::vector<std::vector<std::vector<int32_t>>> m_strategy_sum;
        std::unique_ptr<node_base<N, T>> m_root;
        const game_abstraction_base<N, T>* m_ptr_ga;
        const action_abstraction_base<N>* m_ptr_aa;
        const card_abstraction_base<N, T>* m_ptr_ca;
        //const action_abstraction_base<N, T>* m_ptr_aa;
        // == std::vector<int32_t> m_action_abstraction_sizes;

        cfr_data() = delete;
        cfr_data(std::unique_ptr<node_base<N, T>> root, game_abstraction_base<N, T>* ptr_ga, action_abstraction_base<N>* ptr_aa,
                 card_abstraction_base<N, T>* ptr_ca)
            : m_root(std::move(root)), m_ptr_ga(ptr_ga), m_ptr_aa(ptr_aa), m_ptr_ca(ptr_ca)
        {
            init(m_root.get());
        }

        // print nodes recursively
        void print_strategy_r(const node_base<N, T>* ptr_node, const int level, const int print_level_max = 128) const
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

            const std::vector<std::pair<uint32_t, float>> vec_init;
            std::vector<std::vector<std::pair<uint32_t, float>>> vec_temp(all_actions.size(), vec_init);
            for (uint32_t i = 0; i < m_strategy_sum[gs_id].size(); ++i)
            {
                // skip empty indizes
                if (std::reduce(m_strategy_sum[gs_id][i].cbegin(), m_strategy_sum[gs_id][i].cend()) == 0)
                {
                    continue;
                }

                const auto values = normalize(m_strategy_sum[gs_id][i]);
                uint32_t j = 0;
                //o << space << print_card_abstraction_id(node->m_state, i) << ":\n";
                for (const auto e : values)
                {
                    //vec_temp[j].push_back(std::make_pair(i, e));
                    vec_temp[j].emplace_back(i, e);
                    //o << space << (all_actions[j].str()) << " : " << e << "\n";
                    ++j;
                }
            }

            for (uint32_t i = 0; i < vec_temp.size(); ++i)
            {
                std::cout << space << (all_actions[i].str()) << ": " << std::fixed << std::setprecision(2)
                          << std::accumulate(vec_temp[i].cbegin(), vec_temp[i].cend(), 0.0f,
                                             [](const float val, const auto& e) { return val + e.second; }) *
                                 100 / vec_temp[i].size()
                          << "%\n";
                std::sort(vec_temp[i].begin(), vec_temp[i].end(), [](const auto& lhs, const auto& rhs) { return lhs.second > rhs.second; });
                for (uint32_t j = 0; j < vec_temp[i].size(); ++j)
                {
                    if (j < 25 || j > vec_temp[i].size() - 25)
                        std::cout << space << m_ptr_ca->str_id(ptr_node->m_game_state, vec_temp[i][j].first) << " => "
                                  << vec_temp[i][j].second << "\n";
                }
                std::cout << "\n";
            }
            std::cout << "\n";

            for (auto&& e : ptr_node->m_children)
            {
                if (!e->is_terminal())
                {
                    print_strategy_r(e.get(), level + 1, print_level_max);
                }
            }
        }

       private:
        void init(node_base<N, T>* node)
        {
            // number of actions / children of that node
            const auto inner = std::vector<int32_t>(node->m_children.size(), 0);
            // numer of possible card combinations according to card abstraction
            const auto init_vec = std::vector<std::vector<int32_t>>(m_ptr_ca->size(node->m_game_state), inner);
            m_regret_sum.push_back(init_vec);
            m_strategy_sum.push_back(init_vec);

            for (auto&& child : node->m_children)
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
    std::array<int32_t, 2> cfr_2p(const gamecards<2>& cards, cfr_data<2, uint32_t>& cfrd, node_base<2, uint32_t>* ptr_node,
                                  std::array<float, 2> reach)
    {
        // if the node is terminal, return utility
        //
        if (ptr_node->is_terminal())
        {
            //std::cout << node->m_hash << ": returning " << node->utility_vec(cards)[node->m_active_player] << ", "
            //          << node->utility_vec(cards)[1 - node->m_active_player] << "\n";
            return ptr_node->utility(cards, cfrd.m_ptr_ga);
        }

        // otherwise, call cfr for each action recursively with updated reach for the active player
        //

        // get new strategy
        const auto ap = ptr_node->m_active_player;
        const auto game_abstraction_id = ptr_node->m_id;
        const auto card_abstraction_id = cfrd.m_ptr_ca->id(ptr_node->m_game_state, ptr_node->m_active_player, cards);
        auto strategy = get_strategy(cfrd.m_regret_sum[game_abstraction_id][card_abstraction_id]);
        // update strategy sum
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
            //std::cout << "Xresult (" << std::to_string(node->m_active_player) << ", " << reach_new[node->m_active_player]
            //          << "): " << res[node->m_active_player] << "\n";
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
