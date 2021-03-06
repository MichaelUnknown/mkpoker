/*
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

#pragma once

#include <mkpoker/base/card.hpp>
#include <mkpoker/base/cardset.hpp>
#include <mkpoker/base/hand.hpp>
#include <mkpoker/game/game_def.hpp>
#include <mkpoker/holdem/holdem_evaluation.hpp>
#include <mkpoker/util/array.hpp>
#include <mkpoker/util/mtp.hpp>

#include <algorithm>      // std::find, std::sort
#include <array>          //
#include <cstdint>        //
#include <numeric>        // std::accumulate
#include <span>           //
#include <stdexcept>      //
#include <tuple>          //
#include <type_traits>    // std::enable_if
#include <utility>        // std::pair
#include <vector>         //

namespace mkp
{
    // represents just the cards of a game
    template <std::size_t N, std::enable_if_t<N >= 2 && N <= 6, int> = 0>
    class gamecards
    {
       public:
        const std::array<card, c_num_board_cards> m_board;
        const std::array<hand_2c, N> m_hands;

        ///////////////////////////////////////////////////////////////////////////////////////
        // CTORS
        ///////////////////////////////////////////////////////////////////////////////////////

        // we only allow valid objects
        gamecards() = delete;

        // create with matching arrays
        constexpr explicit gamecards(const std::array<card, c_num_board_cards>& board, const std::array<hand_2c, N>& hands)
            : m_board(board), m_hands(hands)
        {
            if (std::accumulate(m_hands.cbegin(), m_hands.cend(), cardset(board), [](cardset val, const hand_2c elem) {
                    return val.combine(elem.as_cardset());
                }).size() != (2 * N + c_num_board_cards))
            {
                throw std::runtime_error("gamecards(array<card,N>): number of unique cards is not equal to " +
                                         std::to_string(2 * N + c_num_board_cards));
            }
        }

        // create with span of cards (works for any contiguous container)
        constexpr explicit gamecards(const std::span<const card> all_cards)
            : m_board(make_array<card, c_num_board_cards>([&](const uint8_t i) { return all_cards[i]; })),
              m_hands(make_array<hand_2c, N>(
                  [&](const uint8_t i) { return hand_2c(all_cards[2 * i + c_num_board_cards], all_cards[2 * i + c_num_board_cards + 1]); }))
        {
            if (all_cards.size() != (2 * N + c_num_board_cards) || cardset(all_cards).size() != 2 * N + c_num_board_cards)
            {
                throw std::runtime_error("gamecards(span<card>): number of unique cards is not equal to " +
                                         std::to_string(2 * N + c_num_board_cards));
            }
        }

        ///////////////////////////////////////////////////////////////////////////////////////
        // ACCESSORS
        ///////////////////////////////////////////////////////////////////////////////////////

        // getter for n board cards
        [[nodiscard]] auto board_n(unsigned int n) const
        {
            if (n > 5)
            {
                throw std::runtime_error("board_n(unsingned n): index greater than five, i.e. the board size");
            }
            return std::span<const card>(m_board.data(), n);
        }

        // get board as cardset
        [[nodiscard]] auto board_n_as_cs(unsigned int n) const { return cardset(board_n(n)); }

        // debug info
        [[nodiscard]] auto str_cards() const noexcept
        {
            std::string ret{"("};
            for (auto&& e : m_board)
                ret.append(e.str());
            ret.append(") [");
            for (unsigned i = 0; i < N; ++i)
            {
                ret.append("(" + m_hands[i].str() + ")");
                if (i < N - 1)
                    ret.append(",");
            }
            ret.append("]");

            return ret;
        }

        ///////////////////////////////////////////////////////////////////////////////////////
        // MUTATORS
        ///////////////////////////////////////////////////////////////////////////////////////

        // none

        ///////////////////////////////////////////////////////////////////////////////////////
        // helper functions
        ///////////////////////////////////////////////////////////////////////////////////////

        // provide operator for equality

        constexpr auto operator<=>(const gamecards&) const noexcept = delete;
        constexpr bool operator==(const gamecards&) const noexcept = default;
    };

    // class representing a game state without cards
    // chips / stack size are in milli BBs, meaning 1000 equals 1 big blind
    template <std::size_t N, std::enable_if_t<N >= 2 && N <= 6, int> = 0>
    class gamestate
    {
       protected:
        //std::array<int32_t, N> m_chips_start;
        std::array<int32_t, N> m_chips_behind;
        std::array<int32_t, N> m_chips_front;
        std::array<gb_playerstate_t, N> m_playerstate;
        int32_t m_minraise;
        gb_pos_t m_current;
        gb_gamestate_t m_gamestate;

#if !defined(NDEBUG)
        int m_debug_alive = num_alive();
        int m_debug_actionable = num_actionable();
        int m_debug_future = num_future_actionable();
#endif

        // which player starts betting in the first round? heads up: BB(==BTN), otherweise: UTG
        static constexpr auto round0_first_player = N > 2 ? gb_pos_t::UTG : gb_pos_t::BB;

       public:
        ///////////////////////////////////////////////////////////////////////////////////////
        // CTORS
        ///////////////////////////////////////////////////////////////////////////////////////

        // no invalid objects
        gamestate() = delete;

        // create a new game with starting stacksize
        template <std::size_t U = N, std::enable_if_t<U != 2, int> = 0>
        constexpr explicit gamestate(const int32_t stacksize)
            : m_chips_behind(
                  make_array<int32_t, N>([&](std::size_t i) { return i == 0 ? stacksize - 500 : i == 1 ? stacksize - 1000 : stacksize; })),
              m_chips_front(make_array<int32_t, N>([&](std::size_t i) { return i == 0 ? 500 : i == 1 ? 1000 : 0; })),
              m_playerstate(make_array<N>(gb_playerstate_t::INIT)),
              m_minraise(1000),
              m_current(round0_first_player),
              m_gamestate(gb_gamestate_t::PREFLOP_BET)
        {
            if (stacksize < 1000)
            {
                throw std::runtime_error("gamestate(const int): stacksize below 1000 mBB");
            }
        }

        // create a new game with starting stacksize, specialization for heads up
        template <std::size_t U = N, std::enable_if_t<U == 2, int> = 0>
        constexpr explicit gamestate(const int32_t stacksize)
            : m_chips_behind(
                  make_array<int32_t, N>([&](std::size_t i) { return i == 0 ? stacksize - 1000 : i == 1 ? stacksize - 500 : stacksize; })),
              m_chips_front(make_array<int32_t, N>([&](std::size_t i) { return i == 0 ? 1000 : i == 1 ? 500 : 0; })),
              m_playerstate(make_array<N>(gb_playerstate_t::INIT)),
              m_minraise(1000),
              m_current(round0_first_player),
              m_gamestate(gb_gamestate_t::PREFLOP_BET)
        {
            if (stacksize < 1000)
            {
                throw std::runtime_error("gamestate(const int): stacksize below 1000 mBB");
            }
        }

        // create (default) game but with specific starting chip counts
        constexpr gamestate(const std::array<int32_t, N>& chips_start) : gamestate<N>(1000)
        {
            if constexpr (N == 2)
            {
                if (chips_start[0] < 1000 || chips_start[1] < 500)
                {
                    throw std::runtime_error("gamestate(array<int32_t, N>): not enough chips available to post blinds");
                }
            }
            else
            {
                if (chips_start[0] < 500 || chips_start[1] < 1000)
                {
                    throw std::runtime_error("gamestate(array<int32_t, N>): not enough chips available to post blinds");
                }
            }

            // adjust chips
            m_chips_behind = chips_start;
            for (uint8_t i = 0; i < N; ++i)
            {
                m_chips_behind[i] -= m_chips_front[i];
            }
        }

        ///////////////////////////////////////////////////////////////////////////////////////
        // ACCESSORS
        ///////////////////////////////////////////////////////////////////////////////////////

        // is the game finished?
        [[nodiscard]] constexpr bool in_terminal_state() const noexcept { return m_gamestate == gb_gamestate_t::GAME_FIN; }

        // do we have a showdown or did all but one player fold
        [[nodiscard]] constexpr bool is_showdown() const noexcept { return num_alive() > 1; }

        // return current game state
        [[nodiscard]] constexpr gb_gamestate_t gamestate_v() const noexcept { return m_gamestate; }

        // return current player
        [[nodiscard]] constexpr uint8_t active_player() const noexcept { return static_cast<uint8_t>(m_current); }

        // return current player
        [[nodiscard]] constexpr gb_pos_t active_player_v() const noexcept { return m_current; }

        // return player state
        [[nodiscard]] constexpr gb_playerstate_t active_player_state_v() const noexcept { return m_playerstate[active_player()]; }

        // return chip counts
        [[nodiscard]] constexpr std::array<int32_t, N> chips_front() const noexcept { return m_chips_front; }

        // return chip counts
        [[nodiscard]] constexpr std::array<int32_t, N> chips_behind() const noexcept { return m_chips_behind; }

        // helper: highest bet
        [[nodiscard]] constexpr int32_t current_highest_bet() const noexcept
        {
            return *std::max_element(m_chips_front.cbegin(), m_chips_front.cend());
        }

        // helper: chips to call for current player
        [[nodiscard]] constexpr int32_t amount_to_call() const noexcept { return current_highest_bet() - m_chips_front[active_player()]; }

        // helper: total pot size
        [[nodiscard]] constexpr int32_t pot_size() const noexcept
        {
            return std::accumulate(m_chips_front.cbegin(), m_chips_front.cend(), int32_t(0));
        }

        // get alls pots (main pot + every side pot), the vector has the eligible player IDs
        [[nodiscard]] auto all_pots() const -> std::vector<std::tuple<std::vector<unsigned>, int32_t, int32_t>>
        {
            if (!in_terminal_state())
            {
                throw std::runtime_error("all_pots(): game not in terminal state");
            }

            // 1) get all chip counts and sort by amount
            auto chips_and_players =
                make_array<std::pair<int32_t, unsigned>, N>([&](const unsigned idx) { return std::make_pair(m_chips_front[idx], idx); });
            std::sort(chips_and_players.begin(), chips_and_players.end(),
                      [](const auto& lhs, const auto& rhs) { return lhs.first > rhs.first; });

            // 2) we are at showdown, so there must be at least two highest chip counts (main pot)
            int32_t upper = chips_and_players[0].first;
            //std::vector<unsigned> main_pot_players{chips_and_players[0].second, chips_and_players[1].second};
            std::vector<unsigned> main_pot_players;

            // 3) step through the chip counts and check if a player is eligible for the pot or if there is a side pot;
            // for every side pot, there must be one (or more) all in players with less chips
            std::vector<std::tuple<std::vector<unsigned>, int32_t, int32_t>> pots;
            std::for_each(chips_and_players.cbegin(), chips_and_players.cend(), [&](const auto& e) {
                if (m_playerstate[e.second] == gb_playerstate_t::OUT)
                {
                    // skip players that can not win the pot
                }
                else if (const int32_t lower = m_chips_front[e.second]; lower == upper)
                {
                    // same as upper -> add player
                    main_pot_players.push_back(e.second);
                }
                else if (m_playerstate[e.second] == gb_playerstate_t::ALLIN)
                {
                    // found new sidepot -> push the last pot with correct upper/lower
                    pots.emplace_back(std::make_tuple(main_pot_players, upper, lower));
                    upper = lower;
                    // this player is eligible for the sidepot
                    main_pot_players.push_back(e.second);
                }
            });
            // after going over all players, push the last 'unfinished' pot
            pots.emplace_back(std::make_tuple(main_pot_players, upper, 0));

            return pots;
        }

        // return payout on terminal state (only for states with showdown)
        [[nodiscard]] constexpr std::array<int32_t, N> payouts_showdown(const gamecards<N>& cards) const
        {
            if (!in_terminal_state())
            {
                throw std::runtime_error("payouts_showdown(): game not in terminal state");
            }
            if (!is_showdown())
            {
                throw std::runtime_error("payouts_showdown(): terminale state involves no showdown, but cards are given");
            }

            const auto pots = all_pots();
            // return pot_distribution for each (side)pot, add everything up
            return std::accumulate(pots.cbegin(), pots.cend(), std::array<int32_t, N>{}, [&](auto val, const auto& e) {
                return val + pot_distribution(cards, std::get<0>(e), std::get<1>(e), std::get<2>(e));
            });
        }

        // return payout on terminal state (only for states with no showdown required)
        [[nodiscard]] constexpr std::array<int32_t, N> payouts_noshowdown() const
        {
            if (!in_terminal_state())
            {
                throw std::runtime_error("payouts_noshowdown(): game not in terminal state");
            }
            if (is_showdown())
            {
                throw std::runtime_error("payouts_noshowdown(): terminale state involves showdown but no cards are given");
            }

            // winner collects all

            constexpr auto indices = make_array<unsigned, N>(identity{});
            const auto winner = *std::find_if(indices.cbegin(), indices.cend(),
                                              [&](const unsigned idx) { return m_playerstate[idx] != gb_playerstate_t::OUT; });
            return make_array<int32_t, N>([&](const unsigned idx) {
                return idx == winner ? -m_chips_front[idx] + pot_size()    // winner: pot - invested
                                     : -m_chips_front[idx];                // loser: -invested
            });
        }

        // get all possible actions
        [[nodiscard]] std::vector<player_action_t> possible_actions() const noexcept
        {
            std::vector<player_action_t> ret;
            const uint8_t pos = active_player();
            const gb_pos_t pos_t = active_player_v();

            // early exit if player already folded or all in or game finished
            if (m_playerstate[pos] == gb_playerstate_t::OUT || m_playerstate[pos] == gb_playerstate_t::ALLIN ||
                m_gamestate == gb_gamestate_t::GAME_FIN)
            {
                return ret;
            }

            // after early exits, folding should always be legal
            ret.emplace_back(0, gb_action_t::FOLD, pos_t);

            const int32_t highest_bet = current_highest_bet();
            const int32_t chips_committed = m_chips_front[pos];
            const int32_t chips_remaining = m_chips_behind[pos];
            const int32_t chips_total = chips_committed + chips_remaining;

            // is checking legal?
            if (chips_committed == highest_bet)
            {
                ret.emplace_back(0, gb_action_t::CHECK, pos_t);
            }

            // is calling legal?
            // player must not be the highest bidder and have enough chips, keep in mind... calling is not possible,
            // if the players total chips are exactly the highest bet size => all in
            if (chips_committed < highest_bet && chips_total > highest_bet)
            {
                ret.emplace_back(highest_bet - chips_committed, gb_action_t::CALL, pos_t);
            }

            // if there are more chips available, raising / all in is also legal
            // player must not be the highest bidder or in INIT state (someone might have limped)

            // enough chips available to raise?
            if (const int32_t min_raise_size = highest_bet + m_minraise; chips_total > min_raise_size)
            {
                // not highest bidder and either last bet was at least a full raise OR player in INIT state (otherwise no reraise is allowed)
                if (m_playerstate[pos] == gb_playerstate_t::INIT ||
                    (chips_committed < highest_bet && (highest_bet - chips_committed >= m_minraise)))
                {
                    // add all possible raise sizes, stepsize 500mBB
                    for (int32_t current_raise_size = min_raise_size - chips_committed; current_raise_size < chips_remaining;
                         current_raise_size += 500)
                    {
                        ret.emplace_back(current_raise_size, gb_action_t::RAISE, pos_t);
                    }
                }
            }

            // add all in if player has any chips behind
            if (chips_remaining > 0)    // && highest_bet != chips_total)
            {
                // add all in
                ret.emplace_back(chips_remaining, gb_action_t::ALLIN, pos_t);
            }
            return ret;
        }

        // print debug info
        [[nodiscard]] std::string str_state() const noexcept
        {
            std::string ret{"Status: " + to_string(m_gamestate) + ", Pot: " + std::to_string(pot_size()) +
                            ", Minraise: " + std::to_string(m_minraise) + "\n"};
            for (uint8_t i = 0; i < N; ++i)
            {
                if (i % 2 == 0)
                {
                    ret.append(" P" + std::to_string(i) + (i == static_cast<uint8_t>(m_current) ? "*" : " ") + " (" +
                               to_string(m_playerstate[i]) + ", " + std::to_string(m_chips_behind[i]) + ") " +
                               std::to_string(m_chips_front[i]));
                }
                else
                {
                    ret.append(" | " + std::to_string(m_chips_front[i]) + " (" + std::to_string(m_chips_behind[i]) + ", " +
                               to_string(m_playerstate[i]) + ") P" + std::to_string(i) +
                               (i == static_cast<uint8_t>(m_current) ? "*\n" : " \n"));
                }
            }
            return ret;
        }

        ///////////////////////////////////////////////////////////////////////////////////////
        // MUTATORS
        ///////////////////////////////////////////////////////////////////////////////////////

        // update game according to action
        constexpr void execute_action(const player_action_t& pa) noexcept(
#if !defined(NDEBUG)
            false
#else
            true
#endif
        )
        {
#if !defined(NDEBUG)
            // check if active player is a match
            if (m_current != pa.m_pos)
            {
                throw std::runtime_error("execute_action(): active player of action and game state differ");
            }

            // check if action is valid (expensive)
            if (const auto all_actions = possible_actions(); std::find(all_actions.cbegin(), all_actions.cend(), pa) == all_actions.cend())
            {
                throw std::runtime_error("execute_action(): tried to execute invalid action");
            }
#endif

            // adjust chips and player state if necessary
            const uint8_t pos = static_cast<uint8_t>(pa.m_pos);
            switch (pa.m_action)
            {
                case gb_action_t::FOLD:
                    m_playerstate[pos] = gb_playerstate_t::OUT;
                    break;
                case gb_action_t::CHECK:
                    m_playerstate[pos] = gb_playerstate_t::ALIVE;
                    break;
                case gb_action_t::CALL:
                case gb_action_t::RAISE:
                case gb_action_t::ALLIN:
                    if (const int32_t raise_size = pa.m_amount + m_chips_front[pos] - current_highest_bet(); raise_size > m_minraise)
                    {
                        m_minraise = raise_size;
                    }
                    m_chips_behind[pos] -= pa.m_amount;
                    m_chips_front[pos] += pa.m_amount;
                    m_playerstate[pos] = m_chips_behind[pos] == 0 ? gb_playerstate_t::ALLIN : gb_playerstate_t::ALIVE;
                    break;
            }

#if !defined(NDEBUG)
            m_debug_alive = num_alive();
            m_debug_actionable = num_actionable();
            m_debug_future = num_future_actionable();
#endif

            if (const auto num_act = num_actionable(); num_alive() < 2 || (num_act == 0 && num_future_actionable() < 2))
            {
                //
                // the entire hand ended
                //

                // remove unnecessary chips from chips_front if the last call/fold left a player with an unmatched bet
                // this can happen if the last caller does not have enough chips to match the bet and is all in instead
                if (const auto highest_bet = current_highest_bet();
                    std::count(m_chips_front.cbegin(), m_chips_front.cend(), highest_bet) < 2)
                {
                    for (unsigned i = 0; i < N; ++i)
                    {
                        if (m_chips_front[i] == highest_bet)
                        {
                            auto chips = m_chips_front;
                            std::sort(chips.begin(), chips.end(), std::greater{});
                            const int32_t difference = highest_bet - chips[1];    // chips[1]: 2nd highest chip count
                            m_chips_front[i] -= difference;
                            m_chips_behind[i] += difference;
                            break;
                        }
                    }
                }

                m_gamestate = gb_gamestate_t::GAME_FIN;
            }
            else if (num_act == 0)
            {
                //
                // this round ended, reset minbet and player state
                //

                if (m_gamestate == gb_gamestate_t::RIVER_BET)
                {
                    // do nothinng, when the game is over
                    m_gamestate = gb_gamestate_t::GAME_FIN;
                }
                else
                {
                    // active player at round start is always SB unless folded or allin
                    m_current = gb_pos_t::SB;
                    while (m_playerstate[static_cast<uint8_t>(m_current)] == gb_playerstate_t::OUT ||
                           m_playerstate[static_cast<uint8_t>(m_current)] == gb_playerstate_t::ALLIN)
                    {
                        m_current = static_cast<gb_pos_t>((static_cast<uint8_t>(m_current) + 1) % N);
                    }

                    m_minraise = 1000;
                    m_gamestate = static_cast<gb_gamestate_t>(static_cast<int>(m_gamestate) + 1);
                    std::transform(m_playerstate.begin(), m_playerstate.end(), m_playerstate.begin(),
                                   [](const gb_playerstate_t st) { return st == gb_playerstate_t::ALIVE ? gb_playerstate_t::INIT : st; });
                }
            }
            else
            {
                //
                // next players turn
                //

                do
                {
                    m_current = static_cast<gb_pos_t>((static_cast<uint8_t>(m_current) + 1) % N);
                } while (m_playerstate[static_cast<uint8_t>(m_current)] == gb_playerstate_t::OUT ||
                         m_playerstate[static_cast<uint8_t>(m_current)] == gb_playerstate_t::ALLIN);
            }

#if !defined(NDEBUG)
            m_debug_alive = num_alive();
            m_debug_actionable = num_actionable();
            m_debug_future = num_future_actionable();
#endif
        }

        ///////////////////////////////////////////////////////////////////////////////////////
        // helper functions
        ///////////////////////////////////////////////////////////////////////////////////////

        // provide operator for equality

        constexpr auto operator<=>(const gamestate&) const noexcept = delete;
        constexpr bool operator==(const gamestate&) const noexcept = default;

        ///////////////////////////////////////////////////////////////////////////////////////
        // internal helpers
        ///////////////////////////////////////////////////////////////////////////////////////

       protected:
        // players alive (i.e. not OUT)
        [[nodiscard]] constexpr int num_alive() const noexcept
        {
            return std::accumulate(m_playerstate.cbegin(), m_playerstate.cend(), 0, [](const int val, const gb_playerstate_t elem) -> int {
                return elem != gb_playerstate_t::OUT ? val + 1 : val;
            });
        }

        // players who can act (i.e. INIT or ALIVE && able to call/bet)
        [[nodiscard]] constexpr int num_actionable() const noexcept
        {
#if defined(__clang__) || !(defined(__GNUC__) || defined(_MSC_VER))
            // clang 11 does not support c++20 constexpr accumualte yet
            // also exclude other compilers, only gcc and msvc currently support it
            uint8_t ret = 0;
            for (uint8_t index = 0; index < N; index++)
            {
                if (m_playerstate[index] == gb_playerstate_t::INIT ||
                    (m_playerstate[index] == gb_playerstate_t::ALIVE && m_chips_front[index] < current_highest_bet()))
                {
                    ++ret;
                }
            }
            return ret;
#else
            constexpr auto indices = make_array<unsigned, N>(identity{});
            return std::accumulate(indices.cbegin(), indices.cend(), 0, [&](const int val, const unsigned idx) -> int {
                return (m_playerstate[idx] == gb_playerstate_t::INIT ||
                        (m_playerstate[idx] == gb_playerstate_t::ALIVE && m_chips_front[idx] < current_highest_bet()))
                           ? val + 1
                           : val;
            });
#endif
        }

        // players who can act in the next betting round (i.e. not OUT or ALLIN)
        [[nodiscard]] constexpr int num_future_actionable() const noexcept
        {
            return std::accumulate(m_playerstate.cbegin(), m_playerstate.cend(), 0, [](const int val, const gb_playerstate_t elem) -> int {
                return elem != gb_playerstate_t::OUT && elem != gb_playerstate_t::ALLIN ? val + 1 : val;
            });
        }

        // helper: distribute pot according to cards
        [[nodiscard]] auto pot_distribution(const gamecards<N>& cards, const std::vector<unsigned>& eligible_player_indices,
                                            const int32_t upper_bound, const int32_t lower_bound) const -> std::array<int32_t, N>
        {
            // 1) get the winners
            // 1a) start with all possible winners
            std::vector<std::pair<holdem_result, unsigned>> winners;
            std::for_each(eligible_player_indices.cbegin(), eligible_player_indices.cend(), [&](const unsigned idx) {
                winners.emplace_back(evaluate_unsafe(cardset(cards.m_board).combine(cards.m_hands[idx].as_cardset())), idx);
            });
            // 1b) sort by highest hand
            std::sort(winners.begin(), winners.end(), [&](const auto& lhs, const auto& rhs) { return lhs.first > rhs.first; });
            const auto first_non_winner =
                std::find_if(winners.cbegin() + 1, winners.cend(), [&](const auto& e) { return e.first < winners[0].first; });
            const auto dist = std::distance(winners.cbegin(), first_non_winner);
            // 1c) remove non_winners
            // since there is no default ctor for holdem_result, we have to pass a dummy value to resize
            winners.resize(dist, std::make_pair(holdem_result(0, 0, 0, 0), 0));

            // 2) adjust the committed chips, compute winning sum
            // 2a) adjust the committed chips according to lower and upper bound
            const auto chips_front_adjusted = make_array<int32_t, N>([&](const unsigned idx) {
                const int32_t chips = m_chips_front[idx];
                return chips <= lower_bound ? 0 : chips > upper_bound ? upper_bound - lower_bound : chips - lower_bound;
            });
            // 2b) sum_per_winner = sum / (winners.size())
            const int32_t sum_p_winner =
                std::accumulate(chips_front_adjusted.cbegin(), chips_front_adjusted.cend(), 0) / static_cast<int32_t>(winners.size());

            // 3) return payouts for every position according to winners / losers, ignore amounts
            //    chips <= lower ? ignore
            //                   : player_is_a_winner ? add won chips
            //                                        : subtract lost chips
            return make_array<int32_t, N>([&](const unsigned idx) {
                return std::find_if(winners.cbegin(), winners.cend(), [&](const auto& e) { return e.second == idx; }) != winners.cend()
                           ? -chips_front_adjusted[idx] + sum_p_winner
                           : -chips_front_adjusted[idx];    // will return 0 for players who are not involved
            });
        }
    };

}    // namespace mkp