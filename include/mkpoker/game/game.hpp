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
#include <functional>     // std::identity
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
                }).size() != 2 * N + c_num_board_cards)
            {
                throw std::runtime_error("gamecards(array<card,N>): number of unique cards is not equal to " +
                                         std::to_string(2 * N + c_num_board_cards));
            }
        }

        // create with span of cards (works for any contiguous container)
        explicit gamecards(const std::span<const card> all_cards)
            : m_board(make_array<card, c_num_board_cards>([&](const uint8_t i) { return all_cards[i]; })),
              m_hands(make_array<hand_2c, N>(
                  [&](const uint8_t i) { return hand_2c(all_cards[2 * i + c_num_board_cards], all_cards[2 * i + c_num_board_cards + 1]); }))
        {
            if (all_cards.size() != 2 * N + c_num_board_cards || cardset(all_cards).size() != 2 * N + c_num_board_cards)
            {
                throw std::runtime_error("gamecards(array<card,N>): number of unique cards is not equal to " +
                                         std::to_string(2 * N + c_num_board_cards));
            }
        }

        ///////////////////////////////////////////////////////////////////////////////////////
        // ACCESSORS
        ///////////////////////////////////////////////////////////////////////////////////////

        // getter for n board cards
        [[nodiscard]] auto board_n(unsigned int n) const
        {
            if (n > 4)
            {
                throw std::runtime_error("board_n(unsingned n): index greater than board size");
            }
            return std::span<const card>(m_board.data(), n);
        }

        // get board as cardset
        [[nodiscard]] auto board_n_as_cs(unsigned int n) const
        {
            if (n > 4)
            {
                throw std::runtime_error("board_n_as_cs(unsingned n): index greater than board size");
            }
            return cardset(std::span<const card>(m_board.data(), n));
        }

        // debug info
        // todo: N
        [[nodiscard]] auto str_cards() const noexcept
        {
            std::string ret{"("};
            for (auto&& e : m_board)
                ret.append(e.str());
            ret.append(") " + m_hands[0].str() + " <> " + m_hands[1].str());

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
        std::array<int32_t, N> m_chips_start;
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

        ///////////////////////////////////////////////////////////////////////////////////////
        // internal helpers
        ///////////////////////////////////////////////////////////////////////////////////////

        // highest bet
        [[nodiscard]] constexpr int32_t current_highest_bet() const noexcept
        {
            return *std::max_element(m_chips_front.cbegin(), m_chips_front.cend());
        }

        // chips to call for current player
        [[nodiscard]] constexpr int32_t amount_to_call() const noexcept { return current_highest_bet() - m_chips_front[active_player()]; }

        // total pot size for
        [[nodiscard]] constexpr int32_t pot_size() const noexcept
        {
            return std::accumulate(m_chips_front.cbegin(), m_chips_front.cend(), int32_t(0));
        }

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
            constexpr auto indices = make_array<unsigned, N>(std::identity{});
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

       public:
        ///////////////////////////////////////////////////////////////////////////////////////
        // CTORS
        ///////////////////////////////////////////////////////////////////////////////////////

        gamestate() = delete;

        // create a new game with starting stacksize
        template <std::size_t U = N, std::enable_if_t<U != 2, int> = 0>
        constexpr explicit gamestate(const int32_t stacksize)
            : m_chips_start(make_array<N>(stacksize)),
              m_chips_behind(
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
            : m_chips_start(make_array<N>(stacksize)),
              m_chips_behind(
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

        // create a specific game
        constexpr gamestate(const std::array<int32_t, N>& chips_start, const std::array<int32_t, N>& chips_behind,
                            const std::array<int32_t, N>& chips_front, const std::array<gb_playerstate_t, N>& state, const int32_t minraise,
                            const gb_pos_t current, const gb_gamestate_t gamestate)
            : m_chips_start(chips_start),
              m_chips_behind(chips_behind),
              m_chips_front(chips_front),
              m_playerstate(state),
              m_minraise(minraise),
              m_current(current),
              m_gamestate(gamestate)
        {
        }

        ///////////////////////////////////////////////////////////////////////////////////////
        // ACCESSORS
        ///////////////////////////////////////////////////////////////////////////////////////

        // is the game finished?
        [[nodiscard]] constexpr bool in_terminal_state() const noexcept { return m_gamestate == gb_gamestate_t::GAME_FIN; }

        // do we have a showdown or did all but one player fold
        [[nodiscard]] constexpr bool is_showdown() const noexcept { return num_alive() > 1; }

        // return current gamestate
        [[nodiscard]] constexpr gb_gamestate_t gamestate_v() const noexcept { return m_gamestate; }

        // return current player
        [[nodiscard]] constexpr uint8_t active_player() const noexcept { return static_cast<uint8_t>(m_current); }

        // return current player
        [[nodiscard]] constexpr gb_pos_t active_player_v() const noexcept { return m_current; }

        // return player state
        [[nodiscard]] constexpr gb_playerstate_t active_player_state_v() const noexcept { return m_playerstate[active_player()]; }

        // return payout on terminal state (only for states with no showdown required)
        [[nodiscard]] constexpr std::array<int32_t, N> payouts_noshodown() const
        {
            if (!in_terminal_state())
            {
                throw std::runtime_error("payouts_noshodown(): game not in terminal state");
            }
            if (is_showdown())
            {
                throw std::runtime_error("payouts_noshodown(): terminale state involves showdown but no cards are given");
            }

            // winner collects all

            constexpr auto indices = make_array<unsigned, N>(std::identity{});
            const auto winner = *std::find_if(indices.cbegin(), indices.cend(),
                                              [&](const unsigned idx) { return m_playerstate[idx] != gb_playerstate_t::OUT; });
            return make_array<int32_t, N>([&](const unsigned idx) {
                return idx == winner ? -m_chips_front[idx] + pot_size()    // winner: pot - invested
                                     : -m_chips_front[idx];                // loser: -invested
            });
        }

        // return values required for pot size calculations
        [[nodiscard]] constexpr std::pair<int32_t, int32_t> pot_values() const noexcept
        {
            return std::make_pair(pot_size(), amount_to_call());
        }

        // get all possible actions
        [[nodiscard]] std::vector<player_action_t> possible_actions() const noexcept
        {
            std::vector<player_action_t> ret;
            uint8_t pos = active_player();

            // early exit if player already folded or all in or game finished
            if (m_playerstate[pos] == gb_playerstate_t::OUT || m_playerstate[pos] == gb_playerstate_t::ALLIN ||
                m_gamestate == gb_gamestate_t::GAME_FIN)
            {
                return ret;
            }

            // after early exits, folding should always be legal
            ret.emplace_back(0, gb_action_t::FOLD, m_current);

            const int32_t highest_bet = current_highest_bet();
            const int32_t player_local_pot = m_chips_front[pos];
            const int32_t player_total_chips = m_chips_behind[pos];

            // is checking legal?
            if (player_local_pot == highest_bet)
            {
                ret.emplace_back(0, gb_action_t::CHECK, m_current);
            }

            // is calling legal?
            // player must not be the highest bidder and have enough chips
            if (player_local_pot < highest_bet && (player_local_pot + player_total_chips) >= highest_bet)
            {
                ret.emplace_back(highest_bet - player_local_pot, gb_action_t::CALL, m_current);
            }

            // if there are more chips available, raising / all in is also legal
            // player must not be the highest bidder or in INIT state (someone might have limped)

            // enough chips available
            if ((player_local_pot + player_total_chips) > highest_bet)
            {
                // not highest bidder and either last bet was at least a full raise OR player in INIT state (otherwise no reraise is allowed)
                if (m_playerstate[pos] == gb_playerstate_t::INIT ||
                    (player_local_pot < highest_bet && (highest_bet - player_local_pot >= m_minraise)))
                {
                    const int32_t min_raise_size = highest_bet + m_minraise - player_local_pot;
                    const int32_t max_raise_size = player_total_chips;

                    // add all possible raise sizes, stepsize 500mBB
                    for (int32_t current_raise_size = min_raise_size; current_raise_size < max_raise_size; current_raise_size += 500)
                    {
                        ret.push_back({current_raise_size, gb_action_t::RAISE, m_current});
                    }

                    // add all in
                    ret.push_back({max_raise_size, gb_action_t::ALLIN, m_current});
                }
            }

            return ret;
        }

        // print debug info
        [[nodiscard]] std::string str_state() const noexcept
        {
            std::string ret{to_string(m_gamestate) + ":"};

            for (uint8_t ui = 0; ui < N; ++ui)
            {
                if (ui < (N / 2))
                {
                    ret.append(" P" + std::to_string(ui) + (ui == static_cast<uint8_t>(m_current) ? "*" : "") + " (" +
                               to_string(m_playerstate[ui]) + ", " + std::to_string(m_chips_behind[ui]) + ") " +
                               std::to_string(m_chips_front[ui]));
                }
                if (ui == (N / 2))
                {
                    ret.append(" [" + std::to_string(pot_size()) + "]");
                }
                if (ui >= (N / 2))
                {
                    ret.append(" " + std::to_string(m_chips_front[ui]) + " (" + std::to_string(m_chips_behind[ui]) + ", " +
                               to_string(m_playerstate[ui]) + ") P" + std::to_string(ui) +
                               (ui == static_cast<uint8_t>(m_current) ? "*" : ""));
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
            if (const auto all_actions = this->possible_actions();
                std::find(all_actions.cbegin(), all_actions.cend(), pa) == all_actions.cend())
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
                            const int32_t difference = chips[1] - highest_bet;
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
    };

    // combine gamestate and cards, so we can track all data to simulate a game of poker
    // todo: we should probably use composition instead
    template <std::size_t N, std::enable_if_t<N >= 2 && N <= 6, int> = 0>
    class game : public gamestate<N>, public gamecards<N>
    {
       public:
        // create default game with span of cards
        constexpr game(const std::span<const card> all_cards, const int32_t stacksize) : gamestate<N>(stacksize), gamecards<N>(all_cards) {}

        // return payout on showdown
        std::array<int32_t, N> payouts() const
        {
            if (!this->in_terminal_state())
            {
                throw std::runtime_error("payouts(): game not in terminal state");
            }

            if (this->is_showdown())
            {
                // todo: use constexpr if to write optimized versions for 2/3 players

                // get players that are alive / eligible for (part of) the pot
                const std::vector<unsigned> eligible_players = [&] {
                    std::vector<unsigned> result;
                    for (unsigned i = 0; i < N; ++i)
                    {
                        if (this->m_playerstate[i] != gb_playerstate_t::OUT)
                            result.push_back(i);
                    }
                    return result;
                }();

                // most common case: only 2 players for showdown
                // this is just a specialization for two players and could also be handled by the more generic version below
                // however those (3+) cases are actually rather rare and generic algo needs to scan all chip counts for side
                // pots or it would need extra checks to see if there are no side pots
                // also it must use more complex logic, because two, three or more players could share a pot
                if (eligible_players.size() == 2)
                {
                    // evaluate the hands
                    const auto h1 = evaluate_unsafe(cardset(this->m_board).combine(this->m_hands[eligible_players[0]].as_cardset()));
                    const auto h2 = evaluate_unsafe(cardset(this->m_board).combine(this->m_hands[eligible_players[1]].as_cardset()));

                    if (const auto cmp = h1 <=> h2; cmp != 0)
                    {
                        // we have a winner
                        const unsigned pos_winner = cmp > 0 ? eligible_players[0] : eligible_players[1];

                        return make_array<int32_t, N>([&](const unsigned idx) {
                            return idx == pos_winner ? this->pot_size() - this->m_chips_front[idx] : -this->m_chips_front[idx];
                        });
                    }
                    else
                    {
                        // tie
                        const int32_t chips_won = this->pot_size() / 2;

                        return make_array<int32_t, N>([&](const unsigned idx) {
                            return idx == eligible_players[0] || idx == eligible_players[1] ? -this->m_chips_front[idx] + chips_won
                                                                                            : -this->m_chips_front[idx];
                        });
                    }
                }
                else
                {
                    // side pots are possible with > 2

                    // distribute the (side) pot with limits upper and lower
                    // if there is only one pot, the limits are current_highest_bet() and 0
                    // this could also be a private helper function
                    auto pot_distribution = [&](const std::vector<unsigned>& eligible_player_indices, const int32_t upper_bound,
                                                const int32_t lower_bound) -> std::array<int32_t, N> {
                        // 1) sum everything between lower and upper
                        const auto local_pot = std::accumulate(
                            this->m_chips_front.cbegin(), this->m_chips_front.cend(), 0, [&](const int32_t val, const int32_t e) {
                                return val + (e <= lower_bound ? 0 : e > upper_bound ? upper_bound - lower_bound : e - lower_bound);
                            });

                        // 2) get the winners
                        // 2a) start with all possible winners
                        std::vector<std::pair<holdem_evaluation_result, unsigned>> winners;
                        winners.reserve(N);
                        std::for_each(eligible_player_indices.cbegin(), eligible_player_indices.cend(), [&](const unsigned idx) {
                            winners.emplace_back(evaluate_unsafe(cardset(this->m_board).combine(this->m_hands[idx].as_cardset())), idx);
                        });

                        // 2b) sort by highest hand
                        std::sort(winners.begin(), winners.end(), [&](const auto& lhs, const auto& rhs) { return lhs.first > rhs.first; });
                        const auto first_non_winner =
                            std::find_if(winners.cbegin() + 1, winners.cend(), [&](const auto& e) { return e.first < winners[0].first; });
                        const auto dist = std::distance(winners.cbegin(), first_non_winner);
                        // 2c) remove non_winners
                        // since there is no default ctor for holdem_result, we have to pass a dummy value to resize
                        winners.resize(dist, std::make_pair(holdem_evaluation_result(0, 0, 0, 0), 0));

                        // 3) sum_per_winner = sum / (winners.size())
                        const int32_t sum_p_winner = local_pot / static_cast<int32_t>(winners.size());

                        // 4) return payouts for every position according to winners / losers, ignore amounts
                        //    chips <= lower ? ignore
                        //                   : player_is_a_winner ? add won chips
                        //                                        : subtract lost chips

                        return make_array<int32_t, N>([&](const unsigned idx) {
                            return this->m_chips_front[idx] <= lower_bound
                                       ? 0
                                       : (std::find_if(winners.cbegin(), winners.cend(), [&](const auto& e) { return e.second == idx; }) !=
                                          winners.cend())
                                             ? -this->m_chips_front[idx] + lower_bound + sum_p_winner
                                             : -this->m_chips_front[idx] + lower_bound;
                        });
                    };

                    // start| beh |  front
                    // 1000 | 200 | 0: 800 (alive)
                    // 1000 | 200 | 1: 800 (alive)
                    // 1000 | 200 | 6: 800 (alive)
                    // 1000 | 200 | 8: 700 (out)
                    // 1000 | 400 | 4: 600 (allin)
                    // 1000 | 400 | 7: 600 (allin)
                    // 1000 | 550 | 3: 450 (out)
                    // 1000 | 700 | 2: 300 (out)
                    // 1000 |1000 | 5: 0   (out)

                    // get the number of pots as a list of eligible_players, upper, lower bound

                    // 1) get all chip counts and sort by amount
                    auto chips_and_players = make_array<std::pair<int32_t, unsigned>, N>(
                        [&](const unsigned idx) { return std::make_pair(this->m_chips_front[idx], idx); });
                    std::sort(chips_and_players.begin(), chips_and_players.end(),
                              [](const auto& lhs, const auto& rhs) { return lhs.first > rhs.first; });

                    // 2) we are at showdown, so there must be at least two highest chip counts (main pot)
                    int32_t upper = chips_and_players[0].first;
                    std::vector<unsigned> main_pot_players{chips_and_players[0].second, chips_and_players[1].second};

                    // 3) step through the chip counts and check if a player is eligible for the pot or if there is a side pot;
                    // for every side pot, there must be one (or more) all in players with less chips
                    std::vector<std::tuple<std::vector<unsigned>, int32_t, int32_t>> pots;
                    std::for_each(chips_and_players.cbegin() + 2, chips_and_players.cend(), [&](const auto& e) {
                        if (const int32_t lower = this->m_chips_front[e.second]; lower == upper)
                        {
                            // same as upper -> add player
                            main_pot_players.push_back(e.second);
                        }
                        else if (this->m_playerstate[e.second] == gb_playerstate_t::ALLIN)
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

                    // return pot_distribution for each (side)pot, add everything up
                    return std::accumulate(pots.cbegin(), pots.cend(), std::array<int32_t, N>{}, [&](auto val, const auto& e) {
                        return val + pot_distribution(std::get<0>(e), std::get<1>(e), std::get<2>(e));
                    });

                    // return pot_distribution for each (side)pot, add everything up
                    //std::array<int32_t, N> result{};
                    //for (const auto& pot : pots)
                    //{
                    //    const auto tmp = pot_distribution(std::get<0>(pot), std::get<1>(pot), std::get<2>(pot));
                    //    result += tmp;
                    //}
                    //return result;
                }
            }
            else
            {
                return this->payouts_noshodown();
            }
        }
    };

}    // namespace mkp