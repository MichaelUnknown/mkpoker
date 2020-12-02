#pragma once

#include <mkpoker/base/card.hpp>
#include <mkpoker/base/cardset.hpp>
#include <mkpoker/base/hand.hpp>
#include <mkpoker/holdem/holdem_evaluation.hpp>
#include <mkpoker/util/mtp.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <numeric>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace mkp
{
    inline namespace constants
    {
        // positions
        enum class gb_pos_t : uint8_t
        {
            SB = 0,
            BB,
            UTG,
            MP,
            CO,
            BTN
        };

        // possible game states
        enum class gb_gamestate_t : uint8_t
        {
            PREFLOP_BET = 0,
            FLOP_BET,
            TURN_BET,
            RIVER_BET,
            GAME_FIN
        };

        // possible player states
        enum class gb_playerstate_t : uint8_t
        {
            INIT = 0,
            OUT,
            ALIVE,
            ALLIN
        };

        // all possible actions (F/X/C/R/A)
        enum class gb_action_t : uint8_t
        {
            FOLD = 0,
            CHECK,
            CALL,
            RAISE,
            ALLIN
        };

        ///////////////////////////////////////////////////////////////////////////////////////
        // string conversions
        ///////////////////////////////////////////////////////////////////////////////////////

        [[nodiscard]] std::string to_string(const gb_gamestate_t gs)
        {
            switch (gs)
            {
                case gb_gamestate_t::PREFLOP_BET:
                    return std::string("PREFLOP_BET");
                case gb_gamestate_t::FLOP_BET:
                    return std::string("FLOP_BET");
                case gb_gamestate_t::TURN_BET:
                    return std::string("TURN_BET");
                case gb_gamestate_t::RIVER_BET:
                    return std::string("RIVER_BET");
                case gb_gamestate_t::GAME_FIN:
                    return std::string("GAME_FIN");

                default:
                    throw std::runtime_error("to_string(const gb_gamestate_t): invalid game state " +
                                             std::to_string(static_cast<std::underlying_type_t<decltype(gs)>>(gs)));
            }
        }

        [[nodiscard]] std::string to_string(const gb_playerstate_t ps)
        {
            switch (ps)
            {
                case gb_playerstate_t::INIT:
                    return std::string("INIT");
                case gb_playerstate_t::OUT:
                    return std::string("OUT");
                case gb_playerstate_t::ALIVE:
                    return std::string("ALIVE");
                case gb_playerstate_t::ALLIN:
                    return std::string("ALLIN");

                default:
                    throw std::runtime_error("to_string(const gb_playerstate_t): invalid player state " +
                                             std::to_string(static_cast<std::underlying_type_t<decltype(ps)>>(ps)));
            }
        }

        // string conversion
        [[nodiscard]] std::string to_string(const gb_action_t a)
        {
            switch (a)
            {
                case gb_action_t::FOLD:
                    return std::string("FOLD");
                case gb_action_t::CHECK:
                    return std::string("CHECK");
                case gb_action_t::CALL:
                    return std::string("CALL");
                case gb_action_t::RAISE:
                    return std::string("RAISE");
                case gb_action_t::ALLIN:
                    return std::string("ALLIN");

                default:
                    throw std::runtime_error("to_string(const gb_playerstate_t): invalid player state " +
                                             std::to_string(static_cast<std::underlying_type_t<decltype(a)>>(a)));
            }
        }

    }    // namespace constants

    // player_action, struct for logging actions
    struct player_action_t
    {
        int32_t m_amount;
        gb_action_t m_action;
        gb_pos_t m_pos;

        [[nodiscard]] std::string str() const { return to_string(m_action).append("(" + std::to_string(m_amount) + ")"); }

        constexpr auto operator<=>(const player_action_t&) const noexcept = delete;
        constexpr bool operator==(const player_action_t&) const noexcept = default;
    };

    //
    // base representing just the cards
    template <std::size_t N, std::enable_if_t<N >= 2 && N <= 6, int> = 0>
    class gb_cards
    {
       public:
        const std::array<card, 5> m_board;
        const std::array<hand_2c, N> m_hands;

        ///////////////////////////////////////////////////////////////////////////////////////
        // CTORS
        ///////////////////////////////////////////////////////////////////////////////////////

        // we only allow valid objects
        gb_cards() = delete;

        // create with matching arrays
        constexpr explicit gb_cards(const std::array<card, 5>& board, const std::array<hand_2c, N>& hands) : m_board(board), m_hands(hands)
        {
            if (std::accumulate(m_hands.cbegin(), m_hands.cend(), cardset(board), [](cardset val, const hand_2c elem) {
                    return val.combine(elem.as_cardset());
                }).size() != 2 * N + 5)
            {
                throw std::runtime_error("gb_cards(array<card,N>): number of unique cards is not equal to " + std::to_string(2 * N + 5));
            }
        }

        // create with span of cards (works for any contiguous container)
        explicit gb_cards(const std::span<const card> all_cards)
            : m_board(make_array<card, 5>([&](const uint8_t i) { return all_cards[i]; })),
              m_hands(make_array<hand_2c, N>([&](const uint8_t i) { return hand_2c(all_cards[2 * i + 5], all_cards[2 * i + 6]); }))
        {
            if (all_cards.size() != 2 * N + 5 || cardset(all_cards).size() != 2 * N + 5)
            {
                throw std::runtime_error("gb_cards(array<card,N>): number of unique cards is not equal to " + std::to_string(2 * N + 5));
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

        constexpr auto operator<=>(const gb_cards&) const noexcept = delete;
        constexpr bool operator==(const gb_cards&) const noexcept = default;
    };

    //
    // class representing a game state without cards / just state
    // chips / stack size are in milli BBs, meaning 1000 equals 1 big blind
    template <std::size_t N, std::enable_if_t<N >= 2 && N <= 6, int> = 0>
    class gamestate
    {
       protected:
        std::array<int32_t, N> m_chips_start;
        std::array<int32_t, N> m_chips_behind;
        std::array<int32_t, N> m_chips_front;
        std::array<gb_playerstate_t, N> m_playerstate;
        int32_t m_pot;
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
        [[nodiscard]] constexpr int32_t chips_committed() const noexcept
        {
            return std::accumulate(m_chips_front.cbegin(), m_chips_front.cend(), int32_t(0));
        }

        // chips to call for current player
        [[nodiscard]] constexpr int32_t amount_to_call() const noexcept { return current_highest_bet() - m_chips_front[active_player()]; }

        // total pot size for
        [[nodiscard]] constexpr int32_t total_pot_size() const noexcept { return m_pot + chips_committed(); }

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
            const auto indices = make_array<int, N>(std::identity{});
            return std::accumulate(indices.cbegin(), indices.cend(), 0, [&](const int val, const auto index) -> int {
                return (m_playerstate[index] == gb_playerstate_t::INIT ||
                        (m_playerstate[index] == gb_playerstate_t::ALIVE && m_chips_front[index] < current_highest_bet()))
                           ? val + 1
                           : val;
            });
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
        gamestate(const int32_t stacksize)
            : m_chips_start(make_array<N>(stacksize)),
              m_chips_behind(
                  make_array<int32_t, N>([&](std::size_t i) { return i == 0 ? stacksize - 500 : i == 1 ? stacksize - 1000 : stacksize; })),
              m_chips_front(make_array<int32_t, N>([&](std::size_t i) { return i == 0 ? 500 : i == 1 ? 1000 : 0; })),
              m_playerstate(make_array<N>(gb_playerstate_t::INIT)),
              m_pot(0),
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
        gamestate(const int32_t stacksize)
            : m_chips_start(make_array<N>(stacksize)),
              m_chips_behind(
                  make_array<int32_t, N>([&](std::size_t i) { return i == 0 ? stacksize - 1000 : i == 1 ? stacksize - 500 : stacksize; })),
              m_chips_front(make_array<int32_t, N>([&](std::size_t i) { return i == 0 ? 1000 : i == 1 ? 500 : 0; })),
              m_playerstate(make_array<N>(gb_playerstate_t::INIT)),
              m_pot(0),
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
        gamestate(const std::array<int32_t, N>& chips_start, const std::array<int32_t, N>& chips_behind,
                  const std::array<int32_t, N>& chips_front, const std::array<gb_playerstate_t, N>& state, const int32_t pot,
                  const int32_t minraise, const gb_pos_t current, const gb_gamestate_t gamestate)
            : m_chips_start(chips_start),
              m_chips_behind(chips_behind),
              m_chips_front(chips_front),
              m_playerstate(state),
              m_pot(pot),
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

        // return payout on terminal state (only for states with no showdown required)
        [[nodiscard]] constexpr std::array<int32_t, N> effective_payouts() const
        {
            if (!in_terminal_state())
            {
                throw std::runtime_error("effective_payouts(): game not in terminal state");
            }
            if (is_showdown())
            {
                throw std::runtime_error("effective_payouts(): terminale state involves showdown but no cards are given");
            }

            // winner collects all

            // maybe check index_sequence_for...
            // const auto indices = init_array<N, uint8_t>(std::identity{});

            const auto indices = make_array<int, N>(std::identity{});
            const auto winner = *std::find_if(indices.cbegin(), indices.cend(),
                                              [&](const auto index) { return m_playerstate[index] != gb_playerstate_t::OUT; });
            return make_array<int32_t, N>([&](std::size_t i) {
                return i == winner                                                               // invested: "- (chips_start - chips_now)"
                           ? m_pot + chips_committed() + m_chips_behind[i] - m_chips_start[i]    // winner: complete pot - invested
                           : m_chips_behind[i] - m_chips_start[i];                               // loser: -invested
            });
        }

        // return values required for pot size calculations
        [[nodiscard]] constexpr std::pair<int32_t, int32_t> pot_values() const noexcept
        {
            return std::make_pair(total_pot_size(), amount_to_call());
        }

        // get all possible actions
        [[nodiscard]] std::vector<player_action_t> get_possible_actions() const noexcept
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
                    ret.append(" [" + std::to_string(m_pot) + "]");
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
            const uint8_t pos = static_cast<uint8_t>(pa.m_pos);

#if !defined(NDEBUG)
            // check if active player is a match
            if (m_current != pa.m_pos)
            {
                throw std::runtime_error("execute_action(): active player of action and game state differ");
            }

            // check if action is valid (expensive)
            if (const auto all_actions = this->get_possible_actions();
                std::find(all_actions.cbegin(), all_actions.cend(), pa) == all_actions.cend())
            {
                throw std::runtime_error("execute_action(): tried to execute invalid action");
            }
#endif

            // adjust chips and player state if necessary
            switch (pa.m_action)
            {
                case gb_action_t::FOLD:
                    m_pot += m_chips_front[pos];
                    m_chips_front[pos] = 0;
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

}    // namespace mkp