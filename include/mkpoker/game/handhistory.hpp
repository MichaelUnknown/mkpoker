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
#include <mkpoker/util/utility.hpp>

#include <array>        //
#include <cassert>      //
#include <chrono>       // zoned_time, system_clock::now
#include <stdexcept>    // std::runtime_error
#include <string>       //
#include <utility>      // std::pair, std::get
#include <vector>       //

#include <fmt/core.h>    // std::FILE included by fmt

#ifdef _MSC_VER
#include <format>          // std::format with zoned_time
#else                      //
#include <fmt/chrono.h>    // fmt::gmtime
#endif

namespace mkp
{
    // hand history generator with pokerstars.eu style
    template <template <std::size_t... Ns> typename T, std::size_t N, std::size_t... Ns>
    class hh_ps
    {
        //using game_type = T<N, Ns...>;

        constexpr static std::size_t c_num_players = N;
        //constexpr static unsigned c_pos_button = 1u;
        constexpr static unsigned c_pos_button = c_num_players > 2 ? c_num_players : 1u;

        const inline static auto m_timestamp = std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now());

#ifdef _MSC_VER
        // todo: requires c++latest instead of c++20 for now, change build params later
        const inline static std::string m_timestamp_cet_str =
            std::format("{:%Y/%m/%d %H:%M:%S}", std::chrono::zoned_time{"Europe/Berlin", m_timestamp});
        //const inline static std::chrono::zoned_time m_timestamp_cet{"Europe/Berlin", m_timestamp};
        const inline static std::string m_timestamp_et_str =
            std::format("{:%Y/%m/%d %H:%M:%S}", std::chrono::zoned_time{"America/New_York", m_timestamp});
        //const inline static std::chrono::zoned_time m_timestamp_et{"America/New_York", m_timestamp};
#else
        // no support for zoned_time in std::format nor fmt::format
        const inline static std::string m_timestamp_cet_str = fmt::format("{:%Y/%m/%d %H:%M:%S}", fmt::gmtime(m_timestamp));
        const inline static std::string m_timestamp_et_str =
            fmt::format("{:%Y/%m/%d %H:%M:%S}", fmt::gmtime(std::chrono::system_clock::to_time_t(m_timestamp - std::chrono::hours(5))));
#endif

        T<N, Ns...> m_game;
        const gamecards<N> m_cards;
        const std::array<std::string, N> m_names;
        std::vector<std::pair<unsigned, std::string>> m_players_summary;
        const unsigned m_player_id;
        const unsigned m_bb_dollar_ratio;
        const uint64_t m_hand_id;
        std::FILE* m_f;
        gb_gamestate_t m_last_state;

       public:
        ///////////////////////////////////////////////////////////////////////////////////////
        // CTORS
        ///////////////////////////////////////////////////////////////////////////////////////

        // no invalid objects
        hh_ps() = delete;

        hh_ps(const T<N, Ns...>& game, gamecards<N> cards, std::array<std::string, N>& names, std::FILE* f, const unsigned player_id,
              const unsigned bb_dollar_ratio, const uint64_t hand_id)
            : m_game(game),
              m_cards(cards),
              m_names(names),
              m_player_id(player_id),
              m_bb_dollar_ratio(bb_dollar_ratio),
              m_hand_id(hand_id),
              m_f(f),
              m_last_state(m_game.gamestate_v())
        {
            assert(names.size() == N && "size of names must be equal to game size (number of players)");
            // assert that game state is init as the class is created
            assert(game.gamestate_v() == gb_gamestate_t::PREFLOP_BET && "wrong gamestate");
            assert(game.active_player_state_v() == gb_playerstate_t::INIT && "game not in init state");
            assert(game.all_players_state()[0] == gb_playerstate_t::INIT && "game not in init state");
            assert(game.all_players_state()[1] == gb_playerstate_t::INIT && "game not in init state");
            assert(player_id >= 0 && player_id <= N && "player_id does not fit to game size (number of players)");

            // on init, print the base info we already know
            fmt::print(f, "PokerStars Zoom Hand #{:012}:  Hold'em No Limit (${:.2f}/${:.2f}) - {} CET [{} ET]\n", m_hand_id,
                       static_cast<float>(500) / m_bb_dollar_ratio, static_cast<float>(1000) / m_bb_dollar_ratio, m_timestamp_cet_str,
                       m_timestamp_et_str);
            fmt::print(f, "Table '{}' {}-max Seat #{} is the button\n", "Testing", N, c_pos_button);

            for (unsigned int i = 0; i < c_num_players; ++i)
            {
                // native order
                fmt::print(f, "Seat {}: {} (${:.2f} in chips)\n", i + 1, names[i],
                           mbb_to_dollar(game.chips_front()[i] + game.chips_behind()[i]));
            }

            // print blinds and hole cards of m_player_id
            if constexpr (c_num_players > 2)
            {
                fmt::print(f, "{}: posts small blind ${:.2f}\n", names[0], mbb_to_dollar(game.chips_front()[0]));
                fmt::print(f, "{}: posts big blind ${:.2f}\n", names[1], mbb_to_dollar(game.chips_front()[1]));
            }
            else
            {
                fmt::print(f, "{}: posts small blind ${:.2f}\n", names[1], mbb_to_dollar(game.chips_front()[1]));
                fmt::print(f, "{}: posts big blind ${:.2f}\n", names[0], mbb_to_dollar(game.chips_front()[0]));
            }
            fmt::print(f, "*** HOLE CARDS ***\n");
            fmt::print(f, "Dealt to {} [{} {}]\n", names[m_player_id], cards.m_hands[m_player_id].m_card1.str(),
                       cards.m_hands[m_player_id].m_card2.str());
        }

        ///////////////////////////////////////////////////////////////////////////////////////
        // MUTATORS
        ///////////////////////////////////////////////////////////////////////////////////////

        void add_action(const player_action_t& a)
        {
            switch (auto pos = static_cast<unsigned>(a.m_pos); a.m_action)
            {
                case gb_action_t::FOLD:
                    fmt::print(m_f, "{}: folds\n", m_names[pos]);
                    m_players_summary.push_back(
                        std::make_pair(pos, fmt::format("Seat {{}}: {}{} folded {}\n", m_names[pos], str_opt_pos(pos), str_gs_at())));
                    break;

                case gb_action_t::CHECK:
                    fmt::print(m_f, "{}: checks\n", m_names[pos]);
                    break;

                case gb_action_t::CALL:
                    fmt::print(m_f, "{}: calls ${:.2f}\n", m_names[pos], mbb_to_dollar(a.m_amount));
                    break;

                case gb_action_t::RAISE: {
                    auto str_temp = is_bet(a)
                                        ? fmt::format("bets ${:.2f}", mbb_to_dollar(a.m_amount))
                                        : fmt::format("raises ${:.2f} to ${:.2f}",
                                                      mbb_to_dollar(a.m_amount + m_game.chips_front()[pos] - m_game.current_highest_bet()),
                                                      mbb_to_dollar(a.m_amount + m_game.chips_front()[pos]));
                    fmt::print(m_f, "{}: {}\n", m_names[pos], str_temp);

                    break;
                }

                case gb_action_t::ALLIN: {
                    auto str_temp = aa_is_call(a) ? fmt::format("calls ${:.2f}", mbb_to_dollar(a.m_amount))
                                    : is_bet(a)
                                        ? fmt::format("bets ${:.2f}", mbb_to_dollar(a.m_amount))
                                        : fmt::format("raises ${:.2f} to ${:.2f}",
                                                      mbb_to_dollar(a.m_amount + m_game.chips_front()[pos] - m_game.current_highest_bet()),
                                                      mbb_to_dollar(a.m_amount + m_game.chips_front()[pos]));
                    fmt::print(m_f, "{}: {} and is all-in\n", m_names[pos], str_temp);
                    break;
                }

                default:
                    mkp::unreachable();
            }

            // execute action and check if the game state changed
            m_game.execute_action(a);
            if (const auto new_state = m_game.gamestate_v(); new_state != m_last_state)
            {
                if (new_state != gb_gamestate_t::GAME_FIN)
                {
                    m_last_state = new_state;
                    fmt::print(m_f, "{}", str_ccs());
                }
                else
                {
                    // game finished

                    // return money if necessary
                    const auto chips_return = m_game.chips_to_return();
                    if (chips_return.second != 0)
                    {
                        fmt::print(m_f, "Uncalled bet (${:.2f}) returned to {}\n", mbb_to_dollar(chips_return.second),
                                   m_names[static_cast<unsigned>(chips_return.first)]);
                    }

                    const auto adjusted_pot = m_game.pot_size_rake_adjusted();
                    const auto rake = m_game.rake_size();

                    if (m_game.is_showdown())
                    {
                        // showdown

                        switch (m_last_state)
                        {
                            case gb_gamestate_t::PREFLOP_BET:
                                fmt::print(m_f, "*** FLOP *** [{}]\n", str_board(m_cards.m_board, 3));
                                [[fallthrough]];
                            case gb_gamestate_t::FLOP_BET:
                                fmt::print(m_f, "*** TURN *** [{}] [{}]\n", str_board(m_cards.m_board, 3), m_cards.m_board[3].str());
                                [[fallthrough]];
                            case gb_gamestate_t::TURN_BET:
                                fmt::print(m_f, "*** RIVER *** [{}] [{}]\n", str_board(m_cards.m_board, 4), m_cards.m_board[4].str());
                                break;
                        }
                        fmt::print(m_f, "*** SHOW DOWN ***\n");

                        // get all pots, the last pot is always the main pot
                        // get all players from the main pot and print the showdown info
                        const auto pot = m_game.all_pots().back();
                        const auto& vec_ids = std::get<0>(pot);

                        // cards
                        for (const auto& pos : vec_ids)
                        {
                            const auto eval = mkp::evaluate_unsafe(m_cards.board_n_as_cs(5).combine(m_cards.m_hands[pos].as_cardset()));
                            fmt::print(m_f, "{}: shows [{}] ({})\n", m_names[pos], str_hand(m_cards.m_hands[pos]), eval.str());
                        }

                        const auto pot_dist = m_game.pot_distribution(m_cards);

                        // payouts
                        for (const auto& pos : vec_ids)
                        {
                            if (pot_dist[pos] > 0)
                            {
                                fmt::print(m_f, "{} collected ${:.2f} from pot\n", m_names[pos], mbb_to_dollar(pot_dist[pos]));
                            }
                            else if (m_game.chips_behind()[pos] == 0)
                            {
                                fmt::print(m_f, "{} cashed out the hand for ${:.2f}\n", m_names[pos],
                                           mbb_to_dollar(m_game.chips_front()[pos]));
                            }
                        }

                        for (unsigned pos = 0; pos < c_num_players; ++pos)
                        {
                            if (pot_dist[pos] > 0)
                            {
                                // player did win something
                                const auto eval = mkp::evaluate_unsafe(m_cards.board_n_as_cs(5).combine(m_cards.m_hands[pos].as_cardset()));
                                m_players_summary.push_back(std::make_pair(
                                    pos,
                                    fmt::format("Seat {{}}: {}{} showed [{}] and won (${:.2f}) with {}\n", m_names[pos], str_opt_pos(pos),
                                                str_hand(m_cards.m_hands[pos]), mbb_to_dollar(pot_dist[pos]), eval.str())));
                            }
                            else if (pot_dist[pos] <= 0 &&
                                     std::find_if(m_players_summary.cbegin(), m_players_summary.cend(),
                                                  [&](const auto& p) { return p.first == pos; }) == m_players_summary.cend())
                            {
                                // lost, but didn't fold pre
                                const auto eval = mkp::evaluate_unsafe(m_cards.board_n_as_cs(5).combine(m_cards.m_hands[pos].as_cardset()));
                                m_players_summary.push_back(std::make_pair(
                                    pos, fmt::format("Seat {{}}: {}{} showed [{}] and lost with {}{}\n", m_names[pos], str_opt_pos(pos),
                                                     str_hand(m_cards.m_hands[pos]), eval.str(), str_opt_cashout(pos))));
                            }
                        }
                    }
                    else
                    {
                        // no showdown
                        const auto pstate = m_game.all_players_state();
                        const auto it_winner =
                            std::find_if(pstate.cbegin(), pstate.cend(), [](const auto& e) { return e != gb_playerstate_t::OUT; });
                        const auto pos = static_cast<unsigned>(std::distance(pstate.cbegin(), it_winner));

                        fmt::print(m_f, "{} collected ${:.2f} from pot\n", m_names[pos], mbb_to_dollar(adjusted_pot));
                        fmt::print(m_f, "{}: doesn't show hand\n", m_names[pos]);

                        m_players_summary.push_back(std::make_pair(
                            pos, fmt::format("Seat {{}}: {} collected (${:.2f})\n", m_names[pos], mbb_to_dollar(adjusted_pot))));
                    }

                    fmt::print(m_f, "*** SUMMARY ***\n");
                    fmt::print(m_f, "Total pot ${:.2f} | Rake ${:.2f}\n", mbb_to_dollar(adjusted_pot), mbb_to_dollar(rake));
                    fmt::print(m_f, "Board [{}]\n", str_board(m_cards.m_board, 5));

                    // sort the summaries
                    std::sort(m_players_summary.begin(), m_players_summary.end(),
                              [](const auto& lhs, const auto& rhs) { return lhs.first < rhs.first; });
                    for (unsigned int i = 0; i < c_num_players; ++i)
                    {
                        fmt::vprint(m_f, m_players_summary[i].second, fmt::make_format_args(i + 1));
                    }
                    fmt::print(m_f, "\n\n\n");
                }
            }
        }

        ///////////////////////////////////////////////////////////////////////////////////////
        // internal helpers
        ///////////////////////////////////////////////////////////////////////////////////////

       private:
        // convert mbb to dollar
        [[nodiscard]] constexpr float mbb_to_dollar(const int32_t amount) const noexcept
        {
            return static_cast<float>(amount) / m_bb_dollar_ratio;
        }

        // print hand pokerstars style
        [[nodiscard]] MKP_CONSTEXPR_STD_STR std::string str_hand(const hand_2c h) const
        {
            return fmt::format("{} {}", h.m_card1.str(), h.m_card2.str());
        }

        // print board pokerstars style
        [[nodiscard]] MKP_CONSTEXPR_STD_STR std::string str_board(const std::array<card, 5>& b, const unsigned n) const
        {
            return fmt::format("{} {} {}{}{}", b[0].str(), b[1].str(), b[2].str(), n > 3 ? fmt::format(" {}", b[3].str()) : "",
                               n > 4 ? fmt::format(" {}", b[4].str()) : "");
        }

        // print SB, BB or BTN
        [[nodiscard]] MKP_CONSTEXPR_STD_STR std::string str_opt_pos(unsigned i) const
        {
            switch (i)
            {
                case 0:
                    return " (small blind)";
                case 1:
                    return " (big blind)";
                case 5:
                    return " (button)";
                default:
                    return "";
            }
        }

        // cashed out?
        [[nodiscard]] MKP_CONSTEXPR_STD_STR std::string str_opt_cashout(unsigned i) const
        {
            if (m_game.chips_behind()[i] == 0)
            {
                return " (cashed out).";
            }
            else
            {
                return "";
            }
        }

        // used for 'when did a player fold?'
        [[nodiscard]] MKP_CONSTEXPR_STD_STR std::string str_gs_at() const
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
        [[nodiscard]] MKP_CONSTEXPR_STD_STR std::string str_call_or_raise(const player_action_t& a) const
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
        [[nodiscard]] MKP_CONSTEXPR_STD_STR std::string str_ccs() const
        {
            switch (m_game.gamestate_v())
            {
                case gb_gamestate_t::PREFLOP_BET:
                case gb_gamestate_t::GAME_FIN:
                    return {};
                case gb_gamestate_t::FLOP_BET:
                    return fmt::format("*** FLOP *** [{}]\n", str_board(m_cards.m_board, 3));
                case gb_gamestate_t::TURN_BET:
                    return fmt::format("*** TURN *** [{}] [{}]\n", str_board(m_cards.m_board, 3), m_cards.m_board[3].str());
                case gb_gamestate_t::RIVER_BET:
                    return fmt::format("*** RIVER *** [{}] [{}]\n", str_board(m_cards.m_board, 4), m_cards.m_board[4].str());
                default:
                    throw std::runtime_error("str_ccs(): invalid gb_gamestate_t");
            };
        }
    };
}    // namespace mkp