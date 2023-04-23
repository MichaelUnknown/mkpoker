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

#include <mkpoker/cfr/action_abstraction.hpp>
#include <mkpoker/cfr/game_abstraction.hpp>
#include <mkpoker/game/game.hpp>
#include <mkpoker/util/mtp.hpp>

#include <array>
#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

namespace mkp
{
    // With lots of nodes, the goal is to minimize the memory footprint, i.e., keep the node struct
    // as small as possible. So we keep just the id in the node struct + the active player & game
    // state [preflop, flop etc.], which one might need to compute a card abstraction)
    //
    // N = number of players 2..6
    // T = type for hash/id, use uint32_t if possible (i.e., for small games / shallow stack sizes)
    template <std::size_t N, typename T, UnsignedIntegral U = uint32_t>
    struct node_base
    {
        using game_type = T;
        using uint_type = U;
        using encoder_type = game_abstraction_base<T, U>;

        ///////////////////////////////////////////////////////////////////////////////////////
        // DATA
        ///////////////////////////////////////////////////////////////////////////////////////

        std::vector<std::unique_ptr<node_base>> m_children;
        uint_type m_id;                 // hash/id for the gamestate, ui32 or ui64
        gb_gamestate_t m_game_state;    // preflop, flop etc.
        uint8_t m_active_player;        //
        uint8_t m_level;                // tree depth, for debug / printing the tree

        ///////////////////////////////////////////////////////////////////////////////////////
        // CTORS
        ///////////////////////////////////////////////////////////////////////////////////////

        node_base(const uint_type id, const gb_gamestate_t game_state, const uint8_t active_player, const uint8_t level)
            : m_id(id), m_game_state(game_state), m_active_player(active_player), m_level(level)
        {
        }

        virtual ~node_base() = default;

        ///////////////////////////////////////////////////////////////////////////////////////
        // ACCESSORS
        ///////////////////////////////////////////////////////////////////////////////////////

        // is the node a terminal node?
        virtual bool is_terminal() const = 0;

        // utility function for that node
        virtual std::array<int32_t, N> utility(const gamecards<N>& cards, const encoder_type* ptr_enc) const = 0;

        // for debug
        virtual void print_node() const = 0;
    };

    template <std::size_t N, typename T, UnsignedIntegral U = uint32_t>
    struct node_infoset : public node_base<N, T, U>
    {
        using typename node_base<N, T, U>::game_type;
        using typename node_base<N, T, U>::uint_type;
        using typename node_base<N, T, U>::encoder_type;

        ///////////////////////////////////////////////////////////////////////////////////////
        // CTORS
        ///////////////////////////////////////////////////////////////////////////////////////

        node_infoset(const uint_type id, const gb_gamestate_t game_state, const uint8_t active_player, const uint8_t level)
            : node_base<N, T, U>(id, game_state, active_player, level)
        {
        }

        ///////////////////////////////////////////////////////////////////////////////////////
        // ACCESSORS
        ///////////////////////////////////////////////////////////////////////////////////////

        // infoset nodes are never terminal
        virtual bool is_terminal() const override { return false; }

        // infoset nodes do not have a (fix) utility function
        virtual std::array<int32_t, N> utility([[maybe_unused]] const gamecards<N>& cards,
                                               [[maybe_unused]] const encoder_type* ptr_enc) const override
        {
            throw std::runtime_error("node_infoset: utility(...) not available for info set node");
        }

        // print for logging / debug
        virtual void print_node() const override
        {
            for (uint8_t i = 0; i < this->m_level; ++i)
            {
                std::cout << "  ";
            }
            std::cout << "I-Node(" << this->m_id << "), ";
            std::cout << to_string(this->m_game_state) << ", ";
            std::cout << "P:" << std::to_string(this->m_active_player) << " || ";
            std::cout << this->m_children.size() << " children: \n";

            for (const auto& child : this->m_children)
            {
                child->print_node();
            }
        }
    };

    template <std::size_t N, typename T, UnsignedIntegral U = uint32_t>
    struct node_terminal : public node_base<N, T>
    {
        using typename node_base<N, T, U>::game_type;
        using typename node_base<N, T>::uint_type;
        using typename node_base<N, T>::encoder_type;

        ///////////////////////////////////////////////////////////////////////////////////////
        // DATA
        ///////////////////////////////////////////////////////////////////////////////////////

        std::array<int32_t, N> m_payouts;
        bool m_showdown;

        ///////////////////////////////////////////////////////////////////////////////////////
        // CTORS
        ///////////////////////////////////////////////////////////////////////////////////////

        node_terminal(const uint_type id, const gb_gamestate_t game_state, const uint8_t active_player, const uint8_t level,
                      const std::array<int32_t, N>& payouts, const bool showdown)
            : node_base<N, T, U>(id, game_state, active_player, level), m_payouts(payouts), m_showdown(showdown)
        {
        }

        ///////////////////////////////////////////////////////////////////////////////////////
        // ACCESSORS
        ///////////////////////////////////////////////////////////////////////////////////////

        // terminal nodes are terminal
        virtual bool is_terminal() const override { return true; }

        // return utility, if there is no showdown, we return the precomputed payouts
        virtual std::array<int32_t, N> utility(const gamecards<N>& cards, const encoder_type* ptr_enc) const override
        {
            if (!m_showdown)
            {
                return m_payouts;
            }

            return ptr_enc->decode(this->m_id).payouts_showdown(cards);
        }

        // print for logging / debug
        virtual void print_node() const override
        {
            auto array_to_string = [](const std::array<int32_t, N>& arr) -> std::string {
                std::string ret{"[ "};
                for (auto&& e : arr)
                {
                    ret.append(std::to_string(e));
                    ret.append(" ");
                }
                ret.append("]");
                return ret;
            };

            for (uint8_t i = 0; i < this->m_level; ++i)
            {
                std::cout << "  ";
            }
            std::cout << "Terminal(" << this->m_id << "): ";
            std::cout << "showdown: " << std::boolalpha << m_showdown << (m_showdown ? "" : " " + array_to_string(m_payouts)) << "\n";
        }
    };

    // recursively init game tree
    template <template <std::size_t... Ns> typename T, std::size_t N, std::size_t... Ns, UnsignedIntegral U = uint32_t>
    [[nodiscard]] std::unique_ptr<node_base<N, T<N, Ns...>, U>> init_tree(const T<N, Ns...>& gamestate,
                                                                          game_abstraction_base<T<N, Ns...>, U>* ptr_enc,
                                                                          action_abstraction_base<T<N, Ns...>>* ptr_aa,
                                                                          const uint8_t level = 0)
    {
        if (gamestate.in_terminal_state())
        {
            // if there is no showdown, we know the outcome & payouts in advance, otherwise we cannot precompute the result
            if (gamestate.is_showdown())
            {
                std::array<int32_t, N> payouts_unknown{};
                return std::make_unique<node_terminal<N, T<N, Ns...>, U>>(
                    ptr_enc->encode(gamestate),    // id
                    gamestate.gamestate_v(),       // game state, i.e., preflop, flop etc.
                    gamestate.active_player(),     // active player
                    level,                         // depth
                    payouts_unknown,               // payouts unknown w/o cards
                    true);                         // showdown: yes
            }
            else
            {
                return std::make_unique<node_terminal<N, T<N, Ns...>, U>>(
                    ptr_enc->encode(gamestate),        // id
                    gamestate.gamestate_v(),           // game state, i.e., preflop, flop etc.
                    gamestate.active_player(),         // active player
                    level,                             // depth
                    gamestate.payouts_noshowdown(),    // payouts
                    false);                            // showdown: no
            }
        }
        else
        {
            auto info = std::make_unique<node_infoset<N, T<N, Ns...>, U>>(ptr_enc->encode(gamestate),    // id
                                                                          gamestate.gamestate_v(),       // the game state
                                                                          gamestate.active_player(),     // active player
                                                                          level);                        // depth

            const auto actions = ptr_aa->filter_actions(gamestate);
            for (const auto pa : actions)
            {
                auto new_gamestate = gamestate;
                new_gamestate.execute_action(pa);

                info->m_children.push_back(init_tree(new_gamestate, ptr_enc, ptr_aa, static_cast<uint8_t>(level + 1)));
            }
            return info;
        }
    }

}    // namespace mkp
