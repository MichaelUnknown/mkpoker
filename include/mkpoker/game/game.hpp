#pragma once

#include <mkpoker/base/card.hpp>
#include <mkpoker/base/cardset.hpp>
#include <mkpoker/base/hand.hpp>
#include <mkpoker/game/game_def.hpp>
#include <mkpoker/util/mtp.hpp>

#include <algorithm>      // std::find
#include <array>          //
#include <cstdint>        //
#include <functional>     // std::identity
#include <numeric>        // std::accumulate
#include <span>           //
#include <stdexcept>      //
#include <type_traits>    // std::enable_if
#include <vector>         //

namespace mkp
{
    // represents just the cards of a game
    template <std::size_t N, std::enable_if_t<N >= 2 && N <= 6, int> = 0>
    class gamecards
    {
       public:
        const std::array<card, 5> m_board;
        const std::array<hand_2c, N> m_hands;

        ///////////////////////////////////////////////////////////////////////////////////////
        // CTORS
        ///////////////////////////////////////////////////////////////////////////////////////

        // we only allow valid objects
        gamecards() = delete;

        // create with matching arrays
        constexpr explicit gamecards(const std::array<card, 5>& board, const std::array<hand_2c, N>& hands) : m_board(board), m_hands(hands)
        {
            if (std::accumulate(m_hands.cbegin(), m_hands.cend(), cardset(board), [](cardset val, const hand_2c elem) {
                    return val.combine(elem.as_cardset());
                }).size() != 2 * N + 5)
            {
                throw std::runtime_error("gamecards(array<card,N>): number of unique cards is not equal to " + std::to_string(2 * N + 5));
            }
        }

        // create with span of cards (works for any contiguous container)
        explicit gamecards(const std::span<const card> all_cards)
            : m_board(make_array<card, 5>([&](const uint8_t i) { return all_cards[i]; })),
              m_hands(make_array<hand_2c, N>([&](const uint8_t i) { return hand_2c(all_cards[2 * i + 5], all_cards[2 * i + 6]); }))
        {
            if (all_cards.size() != 2 * N + 5 || cardset(all_cards).size() != 2 * N + 5)
            {
                throw std::runtime_error("gamecards(array<card,N>): number of unique cards is not equal to " + std::to_string(2 * N + 5));
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
    // todo: remove pot
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

        // amount chips for pot size calculations
        // todo: delete
        [[nodiscard]] constexpr int32_t chips_committed() const noexcept
        {
            return std::accumulate(m_chips_front.cbegin(), m_chips_front.cend(), int32_t(0));
        }

        // chips to call for current player
        [[nodiscard]] constexpr int32_t amount_to_call() const noexcept { return current_highest_bet() - m_chips_front[active_player()]; }

        // total pot size for
        // todo: rename
        [[nodiscard]] constexpr int32_t total_pot_size() const noexcept
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
            constexpr auto indices = make_array<int, N>(std::identity{});
            return std::accumulate(indices.cbegin(), indices.cend(), 0, [&](const int val, const auto index) -> int {
                return (m_playerstate[index] == gb_playerstate_t::INIT ||
                        (m_playerstate[index] == gb_playerstate_t::ALIVE && m_chips_front[index] < current_highest_bet()))
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

            // maybe check index_sequence_for...
            // const auto indices = init_array<uint8_t, N>(std::identity{});

            const auto indices = make_array<int, N>(std::identity{});
            const auto winner = *std::find_if(indices.cbegin(), indices.cend(),
                                              [&](const auto index) { return m_playerstate[index] != gb_playerstate_t::OUT; });
            return make_array<int32_t, N>([&](std::size_t i) {
                return i == winner                                                      // invested: "- (chips_start - chips_now)"
                           ? total_pot_size() + m_chips_behind[i] - m_chips_start[i]    // winner: complete pot - invested
                           : m_chips_behind[i] - m_chips_start[i];                      // loser: -invested
            });
        }

        // return values required for pot size calculations
        [[nodiscard]] constexpr std::pair<int32_t, int32_t> pot_values() const noexcept
        {
            return std::make_pair(total_pot_size(), amount_to_call());
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

                    // add all possible raise sizes
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
                    ret.append(" [" + std::to_string(total_pot_size()) + "]");
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
    template <std::size_t N, std::enable_if_t<N >= 2 && N <= 6, int> = 0>
    class game : public gamestate<N>, public gamecards<N>
    {
       public:
        // create default game with span of cards
        game(const std::span<const card> all_cards, const int32_t stacksize) : gamestate(stacksize), gamecards(all_cards) {}

        // return payout on showdown
        std::array<int32_t, N> payouts() const
        {
            if (!this->in_terminal_state())
            {
                throw std::runtime_error("payouts_after_shodown(): game not in terminal state");
            }
            return std::array<int32_t, N>();

            // we need this in any case
            const auto chips_invested = this->m_chips_start - this->m_chips_behind;

            if (this->is_showdown())
            {
                // get winner
                //auto h1 = mkp::evaluate_safe(cardset(this->m_board).combine(this->m_cards[0].to_cardset()));
                //auto h2 = mkp::evaluate_safe(cardset(this->m_board).combine(this->m_cards[1].to_cardset()));
                auto h1 = cardset(this->m_board).combine(this->m_cards[0].to_cardset());
                auto h2 = cardset(this->m_board).combine(this->m_cards[1].to_cardset());

                if (h1 == h2)
                {
                    //const auto chips_won = (this->m_chips_front[0] + this->m_chips_front[1] + this->m_pot) / 2;
                    const auto chips_won = (this->m_chips_front[0] + this->m_chips_front[1]) / 2;
                    return (-chips_invested) + chips_won;
                }
                else
                {
                    // chips of loser change ownership
                    const auto pos_loser = h1 > h2 ? 1 : 0;
                    std::array<int32_t, 2> ret_matrix{-1, 1};
                    if (pos_loser == 1)
                    {
                        //ret_matrix *= -1;
                    }
                    ret_matrix *= chips_invested[pos_loser];
                    // do not forget to add the pot
                    //ret_matrix[1 - pos_loser] += this->m_pot;
                    return ret_matrix;
                }
            }
            else
            {
                return this->payouts_noshodown();
            }
        }
    };

}    // namespace mkp