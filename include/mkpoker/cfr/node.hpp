#pragma once

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
    // as small as possible.
    // The current algorithm enumerates the gamestates and keeps just the ID in the node struct (and the
    // active player + game state [preflop, flop etc.], which we need to compute a card / action abstraction)
    //
    // N = number of players 2..6
    // T = integer type, use uint32_t if possible (i.e., for small games / shallow stack sizes)

    template <std::size_t N, typename T = uint32_t>
    struct node_base
    {
        using uint_type = T;

        ///////////////////////////////////////////////////////////////////////////////////////
        // data
        ///////////////////////////////////////////////////////////////////////////////////////

        std::vector<std::unique_ptr<node_base>> m_children;
        uint_type m_id;    // ui32 or ui64
        uint8_t m_gamestate;
        uint8_t m_active_player;
        uint8_t m_level;

        ///////////////////////////////////////////////////////////////////////////////////////
        // CTORS
        ///////////////////////////////////////////////////////////////////////////////////////

        node_base(const uint_type id, const uint8_t gamestate, const uint8_t active_player, const uint8_t level)
            : m_id(id), m_gamestate(gamestate), m_active_player(active_player), m_level(level)
        {
        }

        virtual ~node_base() = default;

        ///////////////////////////////////////////////////////////////////////////////////////
        // ACCESSORS
        ///////////////////////////////////////////////////////////////////////////////////////

        // is the node a terminal node?
        virtual bool is_terminal() const = 0;

        // utility function for that node
        virtual std::array<int32_t, N> utility(const gamecards<N>& cards) const = 0;

        // for debug
        virtual void print_node() const = 0;
    };

    template <std::size_t N, typename T = uint32_t>
    struct node_infoset : public node_base<N, T>
    {
        using uint_type = T;

        ///////////////////////////////////////////////////////////////////////////////////////
        // CTORS
        ///////////////////////////////////////////////////////////////////////////////////////

        node_infoset(const uint_type id, const uint8_t gamestate, const uint8_t active_player, const uint8_t level)
            : node_base(id, gamestate, active_player, level)
        {
        }

        ///////////////////////////////////////////////////////////////////////////////////////
        // ACCESSORS
        ///////////////////////////////////////////////////////////////////////////////////////

        // infoset nodes are never terminal
        virtual bool is_terminal() const override { return false; }

        // infoset nodes do not have a (fix) utility function
        virtual std::array<int32_t, N> utility([[maybe_unused]] const gamecards<N>& cards) const override
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
            std::cout << "I-Node(" << this->m_id << "), " << this->m_children.size() << " children: ";
            std::cout << this->m_gamestate.str_state() << "\n";

            for (const auto& child : this->m_children)
            {
                child->print_node();
            }
        }
    };

    template <std::size_t N, typename T = uint32_t>
    struct node_terminal : public node_base<N, T>
    {
        using uint_type = T;

        ///////////////////////////////////////////////////////////////////////////////////////
        // DATA
        ///////////////////////////////////////////////////////////////////////////////////////

        std::array<int32_t, N> m_payouts;
        bool m_showdown;

        ///////////////////////////////////////////////////////////////////////////////////////
        // CTORS
        ///////////////////////////////////////////////////////////////////////////////////////

        node_terminal(const uint_type id, const uint8_t gamestate, const uint8_t active_player, const uint8_t level,
                      const std::array<int32_t, N>& payouts, const bool showdown)
            : node_base(id, gamestate, active_player, level), m_payouts(payouts), m_showdown(showdown)
        {
        }

        ///////////////////////////////////////////////////////////////////////////////////////
        // ACCESSORS
        ///////////////////////////////////////////////////////////////////////////////////////

        virtual bool is_terminal() const override { return true; }

        virtual std::array<int32_t, N> utility(const gamecards<N>& cards) const override
        {
            if (!m_showdown)
            {
                return m_payouts;
            }

            return magic(this->m_id).payouts_showdown(cards);
        }

        virtual void print_node() const override
        {
            for (uint8_t i = 0; i < this->m_level; ++i)
            {
                std::cout << "  ";
            }
            std::cout << "Terminal(" << this->m_id << "): ";
            std::cout << this->m_gamestate.str_state() << "\n";
        }
    };

    // recursively init game tree
    template <std::size_t N, typename T = uint32_t, typename A = mkp::identity>
    std::unique_ptr<node_base<N, T>> init_tree(const gamestate<N>& gamestate, const uint8_t level = 0, A action_abstraction = A{})
    {
        if (gamestate.in_terminal_state())
        {
            // if there is no showdown, we know the outcome & payouts in advance
            if (gamestate.is_showdown())
            {
                std::array<int32_t, N> payouts_unknown{};
                return std::make_unique<node_terminal<N, T>>(123,                          // id
                                                             gamestate,                    // the gamestate
                                                             gamestate.active_player(),    // active player
                                                             level,                        // depth
                                                             payouts_unknown,              // payouts unknown w/o cards
                                                             true);                        // showdown: yes
            }
            else
            {
                return std::make_unique<node_terminal<N, T>>(123,                               // id
                                                             gamestate,                         // the gamestate
                                                             gamestate.active_player(),         // active player
                                                             level,                             // depth
                                                             gamestate.payouts_noshowdown(),    // payouts
                                                             false);                            // showdown: no
            }
        }
        else
        {
            auto print_actions = [](const std::vector<player_action_t>& actions) {
                for (auto&& a : actions)
                {
                    std::cout << "P:" << to_string(a.m_pos) << " -> ";
                    std::cout << to_string(a.m_action) << " (";
                    std::cout << a.m_amount << ")\n";
                }
                std::cout << "\n";
            };

            auto info = std::make_unique<node_infoset<N, T>>(123,                          // id
                                                             gamestate,                    // the gamestate
                                                             gamestate.active_player(),    // active player
                                                             level);                       // depth

            const auto all_actions = action_abstraction(gamestate.possible_actions());
            print_actions(all_actions);

            for (const auto pa : all_actions)
            {
                auto new_gamestate = gamestate;
                new_gamestate.execute_action(pa);

                info->m_children.push_back(init_tree(new_gamestate, level + 1, action_abstraction));
            }
            return info;
        }
    }

}    // namespace mkp
