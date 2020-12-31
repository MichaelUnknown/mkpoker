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
    // encode gamestate template
    //
    // this is just a blueprint, you can inherit from it but you don't have to
    // the relevant functions are templated and just expect an object with the
    // same api, so you can instead create and use your own type
    template <std::size_t N, typename T = uint32_t>
    struct gamestate_encoder_base
    {
        using uint_type = T;

        virtual ~gamestate_encoder_base() = default;

        virtual uint_type encode(const gamestate<N>& gamestate) = 0;
        virtual gamestate<N> decode(const uint_type i) const = 0;
    };

    // With lots of nodes, the goal is to minimize the memory footprint, i.e., keep the node struct
    // as small as possible. So we keep just the id in the node struct + the active player & game
    // state [preflop, flop etc.], which one might need to compute a card abstraction)
    //
    // N = number of players 2..6
    // T = unsigned integer type, use uint32_t if possible (i.e., for small games / shallow stack sizes)

    template <std::size_t N, typename T = uint32_t, typename E = gamestate_encoder_base<N, T>>
    struct node_base
    {
        using uint_type = T;
        using encoder_type = E;

        ///////////////////////////////////////////////////////////////////////////////////////
        // data
        ///////////////////////////////////////////////////////////////////////////////////////

        std::vector<std::unique_ptr<node_base>> m_children;
        uint_type m_id;    // ui32 or ui64
        uint8_t m_game_state;
        uint8_t m_active_player;
        uint8_t m_level;

        ///////////////////////////////////////////////////////////////////////////////////////
        // CTORS
        ///////////////////////////////////////////////////////////////////////////////////////

        node_base(const uint_type id, const uint8_t game_state, const uint8_t active_player, const uint8_t level)
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
        virtual std::array<int32_t, N> utility(const gamecards<N>& cards, encoder_type* ptr_enc) const = 0;

        // for debug
        virtual void print_node() const = 0;
    };

    template <std::size_t N, typename T = uint32_t, typename E = gamestate_encoder_base<N, T>>
    struct node_infoset : public node_base<N, T, E>
    {
        using node_base<N, T, E>::uint_type;
        using node_base<N, T, E>::encoder_type;

        ///////////////////////////////////////////////////////////////////////////////////////
        // CTORS
        ///////////////////////////////////////////////////////////////////////////////////////

        node_infoset(const uint_type id, const uint8_t game_state, const uint8_t active_player, const uint8_t level)
            : node_base<N, T, E>(id, game_state, active_player, level)
        {
        }

        ///////////////////////////////////////////////////////////////////////////////////////
        // ACCESSORS
        ///////////////////////////////////////////////////////////////////////////////////////

        // infoset nodes are never terminal
        virtual bool is_terminal() const override { return false; }

        // infoset nodes do not have a (fix) utility function
        virtual std::array<int32_t, N> utility([[maybe_unused]] const gamecards<N>& cards,
                                               [[maybe_unused]] encoder_type* ptr_enc) const override
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
            std::cout << to_string(static_cast<gb_gamestate_t>(this->m_game_state)) << ", ";
            std::cout << "P:" << std::to_string(this->m_active_player) << " || ";
            std::cout << this->m_children.size() << " children: \n";

            for (const auto& child : this->m_children)
            {
                child->print_node();
            }
        }
    };

    template <std::size_t N, typename T = uint32_t, typename E = gamestate_encoder_base<N, T>>
    struct node_terminal : public node_base<N, T, E>
    {
        using node_base<N, T, E>::uint_type;
        using node_base<N, T, E>::encoder_type;

        ///////////////////////////////////////////////////////////////////////////////////////
        // DATA
        ///////////////////////////////////////////////////////////////////////////////////////

        std::array<int32_t, N> m_payouts;
        bool m_showdown;

        ///////////////////////////////////////////////////////////////////////////////////////
        // CTORS
        ///////////////////////////////////////////////////////////////////////////////////////

        node_terminal(const uint_type id, const uint8_t game_state, const uint8_t active_player, const uint8_t level,
                      const std::array<int32_t, N>& payouts, const bool showdown)
            : node_base<N, T, E>(id, game_state, active_player, level), m_payouts(payouts), m_showdown(showdown)
        {
        }

        ///////////////////////////////////////////////////////////////////////////////////////
        // ACCESSORS
        ///////////////////////////////////////////////////////////////////////////////////////

        virtual bool is_terminal() const override { return true; }

        virtual std::array<int32_t, N> utility(const gamecards<N>& cards, encoder_type* ptr_enc) const override
        {
            if (!m_showdown)
            {
                return m_payouts;
            }

            return ptr_enc->decode(this->m_id).payouts_showdown(cards);
        }

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
            std::cout << "showdown: " << std::boolalpha << m_showdown << (m_showdown ? "" : array_to_string(m_payouts)) << "\n";
        }
    };

    // recursively init game tree
    template <std::size_t N, typename T = uint32_t, typename E = gamestate_encoder_base<N, T>, typename A = mkp::identity>
    std::unique_ptr<node_base<N, T, E>> init_tree(const gamestate<N>& gamestate, E* ptr_enc, const uint8_t level = 0,
                                                  A action_abstraction = A{})
    {
        if (gamestate.in_terminal_state())
        {
            // if there is no showdown, we know the outcome & payouts in advance
            if (gamestate.is_showdown())
            {
                std::array<int32_t, N> payouts_unknown{};
                return std::make_unique<node_terminal<N, T, E>>(
                    ptr_enc->encode(gamestate),                       // id
                    static_cast<uint8_t>(gamestate.gamestate_v()),    // game state, i.e., preflop, flop etc.
                    gamestate.active_player(),                        // active player
                    level,                                            // depth
                    payouts_unknown,                                  // payouts unknown w/o cards
                    true);                                            // showdown: yes
            }
            else
            {
                return std::make_unique<node_terminal<N, T, E>>(
                    ptr_enc->encode(gamestate),                       // id
                    static_cast<uint8_t>(gamestate.gamestate_v()),    // game state, i.e., preflop, flop etc.
                    gamestate.active_player(),                        // active player
                    level,                                            // depth
                    gamestate.payouts_noshodown(),                    // payouts
                    false);                                           // showdown: no
            }
        }
        else
        {
            auto info = std::make_unique<node_infoset<N, T, E>>(ptr_enc->encode(gamestate),                       // id
                                                                static_cast<uint8_t>(gamestate.gamestate_v()),    // the gamestate
                                                                gamestate.active_player(),                        // active player
                                                                level);                                           // depth

            const auto all_actions = action_abstraction(gamestate.possible_actions());

            for (const auto pa : all_actions)
            {
                auto new_gamestate = gamestate;
                new_gamestate.execute_action(pa);

                info->m_children.push_back(init_tree(new_gamestate, ptr_enc, static_cast<uint8_t>(level + 1), action_abstraction));
            }
            return info;
        }
    }

}    // namespace mkp
