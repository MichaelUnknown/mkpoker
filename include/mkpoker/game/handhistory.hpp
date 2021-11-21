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

#include <mkpoker/game/game.hpp>

#include <algorithm>
#include <array>
#include <cassert>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <fmt/core.h>

namespace mkp
{
    // hand history generator with pokerstars.eu style
    template <template <std::size_t... Ns> typename T, std::size_t N, std::size_t... Ns>
    class hh_ps
    {
        //using game_type = T<N, Ns...>;

        constexpr static auto c_num_players = N;
        constexpr static auto c_pos_button = c_num_players > 2 ? 5u : 1u;

        T<N, Ns...> m_game;
        const gamecards<N> m_cards;
        const std::array<std::string, N> m_names;
        //std::vector<player_action_t> m_actions;
        std::vector<std::pair<unsigned, std::string>> m_player_resume;
        std::FILE* m_f;
        const unsigned m_player_id;
        const unsigned m_bb_dollar_ratio;
        gb_gamestate_t m_last_state;

       public:
        ///////////////////////////////////////////////////////////////////////////////////////
        // CTORS
        ///////////////////////////////////////////////////////////////////////////////////////
        // no invalid objects
        hh_ps() = delete;

        hh_ps(const T<N, Ns...>& game, gamecards<N> cards, std::array<std::string, N>& names, std::FILE* f, const unsigned player_id,
              const unsigned bb_dollar_ratio)
            : m_game(game),
              m_cards(cards),
              m_names(names),
              m_f(f),
              m_player_id(player_id),
              m_bb_dollar_ratio(bb_dollar_ratio),
              m_last_state(m_game.gamestate_v())
        {
            // assert that span has correct size
            assert(names.size() == N && "size of names must be equal to game size (number of players)");
            // assert that game state is init as the class is created
            assert(game.gamestate_v() == gb_gamestate_t::PREFLOP_BET && "wrong gamestate");
            assert(game.active_player_state_v() == gb_playerstate_t::INIT && "game not in init state");
            assert(game.all_players_state()[0] == gb_playerstate_t::INIT && "game not in init state");
            assert(game.all_players_state()[1] == gb_playerstate_t::INIT && "game not in init state");
            assert(player_id >= 0 && player_id <= N && "player_id does not fit to game size (number of players)");

            // on init, print the base info we already know
            fmt::print(f, "ratio: {}\n", bb_dollar_ratio);
            fmt::print(f, "Table '{}' {}-max Seat #{} is the button\n", "Testing", N, c_pos_button);
            for (unsigned int i = 0; i < c_num_players; ++i)
            {
                fmt::print(f, "Seat {}: {} (${} in chips)\n", i, names[i], game.chips_front()[i] + game.chips_behind()[i]);
            }
            if constexpr (c_num_players > 2)
            {
                fmt::print(f, "{}: posts small blind ${}\n", names[0], game.chips_front()[0]);
                fmt::print(f, "{}: posts big blind ${}\n", names[1], game.chips_front()[1]);
            }
            else
            {
                fmt::print(f, "{}: posts small blind ${}\n", names[1], game.chips_front()[1]);
                fmt::print(f, "{}: posts big blind ${}\n", names[0], game.chips_front()[0]);
            }
            fmt::print(f, "*** HOLE CARDS ***\n");

            fmt::print(f, "Dealt to {} [{} {}]\n", names[player_id], cards.m_hands[player_id].m_card1.str(),
                       cards.m_hands[player_id].m_card2.str());
        }

        ///////////////////////////////////////////////////////////////////////////////////////
        // MUTATORS
        ///////////////////////////////////////////////////////////////////////////////////////

        void add_action(const player_action_t& a)
        {
            //m_actions.push_back(a);

            switch (auto pos = static_cast<unsigned>(a.m_pos); a.m_action)
            {
                case gb_action_t::FOLD:
                    fmt::print(m_f, "{}: folds\n", m_names[pos]);
                    m_player_resume.push_back(std::make_pair(pos, fmt::format("Seat {}: {} folded {}\n", pos, m_names[pos], str_gs_at())));
                    break;

                case gb_action_t::CHECK:
                    fmt::print(m_f, "{}: checks\n", m_names[pos]);
                    break;

                case gb_action_t::CALL:
                    fmt::print(m_f, "{}: calls ${}\n", m_names[pos], a.m_amount);
                    break;

                case gb_action_t::RAISE: {
                    auto str_temp =
                        is_bet(a) ? fmt::format("bets ${}", a.m_amount)
                                  : fmt::format("raises ${} to ${}", a.m_amount + m_game.chips_front()[pos] - m_game.current_highest_bet(),
                                                a.m_amount + m_game.chips_front()[pos]);
                    fmt::print(m_f, "{}: {}\n", m_names[pos], str_temp);

                    break;
                }

                case gb_action_t::ALLIN: {
                    //const auto aa_call = aa_is_call(a);
                    //if (aa_call)
                    //{
                    //    fmt::print(m_f, "{}: calls ${} and is all-in\n", m_names[pos], m_game.chips_behind()[pos],
                    //               a.m_amount + m_game.chips_front()[pos]);
                    //}
                    //else
                    //{
                    //    auto str_temp = is_bet(a) ? fmt::format("bets ${}", a.m_amount)
                    //                              : fmt::format("raises ${} to ${}",
                    //                                            a.m_amount + m_game.chips_front()[pos] - m_game.current_highest_bet(),
                    //                                            a.m_amount + m_game.chips_front()[pos]);
                    //}

                    auto str_temp = aa_is_call(a) ? fmt::format("calls ${}", a.m_amount)
                                    : is_bet(a)   ? fmt::format("bets ${}", a.m_amount)
                                                  : fmt::format("raises ${} to ${}",
                                                              a.m_amount + m_game.chips_front()[pos] - m_game.current_highest_bet(),
                                                              a.m_amount + m_game.chips_front()[pos]);
                    fmt::print(m_f, "{}: {} and is all-in\n", m_names[pos], str_temp);
                    break;
                }

                    //default:
                    //    __assume(0);
                    //    break;
            }

            m_game.execute_action(a);
            if (const auto new_state = m_game.gamestate_v(); new_state != m_last_state)
            {
                if (new_state != gb_gamestate_t::GAME_FIN)
                {
                    m_last_state = new_state;
                    fmt::print(m_f, str_ccs());
                }
                else
                {
                    // is there a showdown?
                    if (m_game.is_showdown())
                    {
                        fmt::print(m_f, "*** SHOW DOWN ***\n");
                    }
                    else
                    {
                        // no showdown
                        const auto pstate = m_game.all_players_state();
                        const auto it_winner =
                            std::find_if(pstate.cbegin(), pstate.cend(), [](const auto& e) { return e != gb_playerstate_t::OUT; });
                        const auto pos = static_cast<unsigned>(std::distance(pstate.cbegin(), it_winner));
                        m_player_resume.push_back(
                            std::make_pair(pos, fmt::format("Seat {}: {} collected (${:.2f})\n", pos, m_names[pos], 142.13)));
                    }

                    fmt::print(m_f, "*** SUMMARY ***\n");
                    fmt::print(m_f, "Total pot $2 | Rake $0.09\n");
                    fmt::print(m_f, "Board [7s 4c 5s 4h Ks]\n");

                    std::sort(m_player_resume.begin(), m_player_resume.end(),
                              [](const auto& lhs, const auto& rhs) { return lhs.first < rhs.first; });
                    for (auto e : m_player_resume)
                    {
                        fmt::print(m_f, e.second);
                    }
                    fmt::print(m_f, "\n\n\n");
                }
            }
        }

        ///////////////////////////////////////////////////////////////////////////////////////
        // internal helpers
        ///////////////////////////////////////////////////////////////////////////////////////

       private:
        // used for 'when did a player fold?'
        [[nodiscard]] constexpr std::string str_gs_at() const
        {
            switch (m_game.gamestate_v())
            {
                case gb_gamestate_t::PREFLOP_BET:
                    return fmt::format("before Flop{}",
                                       m_game.chips_front()[static_cast<unsigned>(m_game.active_player())] == 0 ? " (didn't bet)" : "");

                case gb_gamestate_t::FLOP_BET:
                    return "on the Flop";

                case gb_gamestate_t::TURN_BET:
                    return "on the Turn";

                case gb_gamestate_t::RIVER_BET:
                    return "on the River";

                default:
                    throw std::runtime_error("str_gs_at(): invalid gb_gamestate_t");
                    //    __assume(0);
                    //    return "";
            }
        }

        // bet or raise?
        [[nodiscard]] constexpr bool is_bet(const player_action_t& a) const
        {
            if (m_game.current_highest_bet() == m_game.chips_front()[static_cast<unsigned>(a.m_pos)])
            {
                return true;
            }
            else
            {
                return false;
            }
        }

        // is the all-in a raise or call?
        [[nodiscard]] constexpr bool aa_is_call(const player_action_t& a) const
        {
            if (m_game.current_highest_bet() < (a.m_amount + m_game.chips_front()[static_cast<unsigned>(a.m_pos)]))
            {
                return false;
            }
            else
            {
                return true;
            }
        }

        // is the all-in a raise or call?
        [[nodiscard]] constexpr std::string str_call_or_raise(const player_action_t& a) const
        {
            if (m_game.current_highest_bet() < (a.m_amount + m_game.chips_front()[static_cast<unsigned>(a.m_pos)]))
            {
                return "raises";
            }
            else
            {
                return "calls";
            }
        }

        // community cards
        [[nodiscard]] constexpr std::string str_ccs() const
        {
            switch (m_game.gamestate_v())
            {
                case gb_gamestate_t::PREFLOP_BET:
                case gb_gamestate_t::GAME_FIN:
                    return {};
                case gb_gamestate_t::FLOP_BET:
                    return fmt::format("*** FLOP *** [{} {} {}]\n", m_cards.m_board[0].str(), m_cards.m_board[1].str(),
                                       m_cards.m_board[2].str());
                case gb_gamestate_t::TURN_BET:
                    return fmt::format("*** TURN *** [{} {} {}] [{}]\n", m_cards.m_board[0].str(), m_cards.m_board[1].str(),
                                       m_cards.m_board[2].str(), m_cards.m_board[3].str());
                case gb_gamestate_t::RIVER_BET:
                    return fmt::format("*** RIVER *** [{} {} {} {}] [{}]\n", m_cards.m_board[0].str(), m_cards.m_board[1].str(),
                                       m_cards.m_board[2].str(), m_cards.m_board[3].str(), m_cards.m_board[4].str());
                default:
                    throw std::runtime_error("str_ccs(): invalid gb_gamestate_t");
            };
        }
    };
}    // namespace mkp