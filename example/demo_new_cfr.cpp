#include <mkpoker/cfr/action_abstraction.hpp>
#include <mkpoker/cfr/card_abstraction.hpp>
#include <mkpoker/cfr/cfr.hpp>
#include <mkpoker/cfr/game_abstraction.hpp>
#include <mkpoker/cfr/node.hpp>
#include <mkpoker/game/game.hpp>
#include <mkpoker/util/card_generator.hpp>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

// #include <nlohmann/json.hpp>

constexpr auto c_num_players = 2;

// namespace mkp
// {

//     // return node as json
//     template <std::size_t N, typename T, UnsignedIntegral U>
//     auto json_node(const mkp::node_base<N, T, U>* node,
//                    const mkp::gamestate_enumerator<mkp::gamestate<c_num_players, 0, 1>, uint32_t>& game_abstraction) -> nlohmann::json
//     {
//         const auto gs = game_abstraction.decode(node->m_id).gamestate_v();

//         if (node->is_terminal())
//         {
//             return nlohmann::json{{"nodegameId", node->m_id}, {"nodegameInfo", fmt::format("{}/{}", node->m_id, mkp::to_string(gs))}};
//         }
//         else
//         {
//             auto children = nlohmann::json::array();
//             for (auto&& child : node->m_children)
//             {
//                 children.push_back(mkp::json_node(child.get(), game_abstraction));
//             }
//             const auto str_active_player =
//                 node->m_game_state == mkp::gb_gamestate_t::GAME_FIN ? "" : fmt::format(" (active player: {})", node->m_active_player);
//             return nlohmann::json{{"nodegameId", node->m_id},
//                                   {"nodegameInfo", fmt::format("{}/{}{}", node->m_id, mkp::to_string(gs), str_active_player)},
//                                   {"nodeChildren", children}};
//         }
//     }

// }    // namespace mkp

struct hand_flop_cs_t
{
    mkp::cardset h;
    mkp::cardset f;
    constexpr auto operator<=>(const hand_flop_cs_t&) const noexcept = default;
};

// card abstraction for preflop (by range)
template <std::size_t N, mkp::UnsignedIntegral T = uint32_t>
struct card_abstraction_norm_pf_flop_only final : public mkp::card_abstraction_base<N, T>
{
    using typename mkp::card_abstraction_base<N, T>::uint_type;

    std::vector<mkp::cardset> hands_preflop;
    std::vector<hand_flop_cs_t> hands_flops_cs;

    card_abstraction_norm_pf_flop_only()
    {
        // preflop
        hands_preflop.reserve(1326);
        {
            for (unsigned i = 0; i < mkp::c_deck_size; ++i)
            {
                for (unsigned j = i + 1; j < mkp::c_deck_size; ++j)
                {
                    const mkp::cardset hand{mkp::make_bitset(i, j)};
                    hands_preflop.emplace_back(hand.rotate_suits(hand.get_normalization_vector()));
                }
            }
        }
        fmt::print("unique hands preflop: {}\n", hands_preflop.size());
        std::ranges::sort(hands_preflop);
        {
            const auto ret = std::ranges::unique(hands_preflop);
            hands_preflop.erase(ret.begin(), ret.end());
        }
        fmt::print("unique hands preflop after unique: {}\n", hands_preflop.size());

        // flop
        hands_flops_cs.reserve(25'989'600);
        {
            // store all the different hand/flop combos
            for (unsigned i = 0; i < mkp::c_deck_size; ++i)
            {
                for (unsigned j = i + 1; j < mkp::c_deck_size; ++j)
                {
                    const mkp::cardset hand{mkp::make_bitset(i, j)};

                    for (unsigned k = 0; k < mkp::c_deck_size; ++k)
                    {
                        mkp::card c3{static_cast<uint8_t>(k)};
                        if (hand.contains(c3))
                        {
                            continue;
                        }
                        for (unsigned l = k + 1; l < mkp::c_deck_size; ++l)
                        {
                            mkp::card c4{static_cast<uint8_t>(l)};
                            if (hand.contains(c4))
                            {
                                continue;
                            }
                            for (unsigned m = l + 1; m < mkp::c_deck_size; ++m)
                            {
                                mkp::card c5{static_cast<uint8_t>(m)};
                                if (hand.contains(c5))
                                {
                                    continue;
                                }
                                const mkp::hand_2c hand{mkp::card{static_cast<uint8_t>(i)}, mkp::card{static_cast<uint8_t>(j)}};
                                const mkp::cardset flop{c3, c4, c5};

                                const auto arr = suit_normalization_permutation(hand, flop);

                                hands_flops_cs.emplace_back(
                                    mkp::cardset{mkp::card{static_cast<uint8_t>(i)}, mkp::card{static_cast<uint8_t>(j)}}.rotate_suits(arr),
                                    flop.rotate_suits(arr));
                            }
                        }
                    }
                }
            }
            fmt::print("unique hand/flop combos: {}\n", hands_flops_cs.size());
        }

        std::ranges::sort(hands_flops_cs);
        {
            const auto ret = std::ranges::unique(hands_flops_cs);
            hands_flops_cs.erase(ret.begin(), ret.end());
        }
        fmt::print("unique hand/flop combos after unique: {}\n", hands_flops_cs.size());
    }

    // bucket size
    [[nodiscard]] virtual uint_type size(const mkp::gb_gamestate_t game_state) const override
    {
        if (game_state == mkp::gb_gamestate_t::PREFLOP_BET)
        {
            return hands_preflop.size();
        }
        else if (game_state == mkp::gb_gamestate_t::FLOP_BET)
        {
            return hands_flops_cs.size();
        }
        else
        {
            return 1;
        }
    }

    [[nodiscard]] uint_type id_impl(const mkp::gb_gamestate_t game_state, const mkp::hand_2c& hand, const mkp::cardset& board_cards) const
    {
        if (game_state == mkp::gb_gamestate_t::PREFLOP_BET)
        {
            const auto hand_cs = hand.as_cardset();
            const auto hand_cs_norm = hand_cs.rotate_suits(hand_cs.get_normalization_vector());
            if (const auto it = std::find(hands_preflop.cbegin(), hands_preflop.cend(), hand_cs_norm); it != hands_preflop.cend())
            {
                return static_cast<uint_type>(std::distance(hands_preflop.cbegin(), it));
            }
            else
            {
                std::cout << "could not find id for hand/board [preflop]: " << hand.str() << "/" << board_cards.str()
                          << " (normalized: " << hand_cs_norm.str() << ")\n";
                return 0;
            }
            // return std::distance(hands_preflop.cbegin(),
            //                      std::find(hands_preflop.cbegin(), hands_preflop.cend(), hand.as_cardset()));
        }
        else if (game_state == mkp::gb_gamestate_t::FLOP_BET)
        {
            return 0;

            const auto arr = suit_normalization_permutation(hand, board_cards);
            const auto norm_h_cs = hand.as_cardset().rotate_suits(arr);
            const auto norm_f_cs = board_cards.rotate_suits(arr);

            if (const auto it = std::find(hands_flops_cs.cbegin(), hands_flops_cs.cend(), hand_flop_cs_t{norm_h_cs, norm_f_cs});
                it != hands_flops_cs.cend())
            {
                return static_cast<uint_type>(std::distance(hands_flops_cs.cbegin(), it));
            }
            else
            {
                std::cout << "could not find id for hand/board [flop]: " << hand.str() << "/" << board_cards.str()
                          << " (normalized: " << norm_h_cs.str() << "|" << norm_f_cs.str() << ")\n";
                return 0;
            }
        }
        else
        {
            return 0;
        }
    }

    // bucket id
    [[nodiscard]] virtual uint_type id(const mkp::gb_gamestate_t game_state, const uint8_t active_player,
                                       const mkp::gamecards<N>& cards) const override
    {
        return id_impl(game_state, cards.m_hands[active_player], cards.board_n_as_cs(5));
    }

    // // json data of id / bucket
    // [[nodiscard]] auto str_id_json(const mkp::gb_gamestate_t game_state, uint_type id) const
    // {
    //     if (game_state == mkp::gb_gamestate_t::PREFLOP_BET)
    //     {
    //         // export interface PokerActionCardBucket {
    //         //     handCards : Array<Card>;
    //         //     boardCards : Array<Card>;
    //         // }

    //         const auto cards = hands_preflop[id].as_cards();
    //         auto cardsHandJson = nlohmann::json::array();
    //         for (auto&& card : cards)
    //         {
    //             cardsHandJson.push_back(nlohmann::json{{"rank", card.rank().str()}, {"suit", card.suit().str()}});
    //         }
    //         return nlohmann::json{{"handCards", cardsHandJson}, {"boardCards", nlohmann::json::array()}};
    //     }
    //     else if (game_state == mkp::gb_gamestate_t::FLOP_BET)

    //     {
    //         const auto hand_and_flop = hands_flops_cs[id];
    //         const auto cardsHand = hand_and_flop.h.as_cards();
    //         const auto cardsFlop = hand_and_flop.f.as_cards();
    //         auto cardsHandJson = nlohmann::json::array();
    //         auto cardsBoardJson = nlohmann::json::array();
    //         for (auto&& card : cardsHand)
    //         {
    //             cardsHandJson.push_back(nlohmann::json{{"rank", card.rank().str()}, {"suit", card.suit().str()}});
    //         }
    //         for (auto&& card : cardsFlop)
    //         {
    //             cardsBoardJson.push_back(nlohmann::json{{"rank", card.rank().str()}, {"suit", card.suit().str()}});
    //         }
    //         return nlohmann::json{{"handCards", cardsHandJson}, {"boardCards", cardsBoardJson}};
    //     }
    //     else
    //     {
    //         return nlohmann::json{{"handCards", nlohmann::json::array()}, {"boardCards", nlohmann::json::array()}};
    //     }
    // }

    // debug / human readable description of that id / bucket
    [[nodiscard]] virtual std::string str_id(const mkp::gb_gamestate_t game_state, uint_type id) const override
    {
        if (game_state == mkp::gb_gamestate_t::PREFLOP_BET)
        {
            return hands_preflop[id].str();
        }
        else if (game_state == mkp::gb_gamestate_t::FLOP_BET)
        {
            const auto hand_flop = hands_flops_cs[id];
            return hand_flop.h.str() + " | " + hand_flop.f.str();
        }
        else
        {
            return "only one default bucket for turn + river";
        }
    }
};

// allows only a limited amount of bet sizes (33%/66%/100%/133% pot) and
// checking down after the flop (i.e., no turn or river play)
template <typename T>
struct action_abstraction_pf_flop_only final : public mkp::action_abstraction_base<T>
{
    using typename mkp::action_abstraction_base<T>::game_type;

    [[nodiscard]] virtual std::vector<mkp::player_action_t> filter_actions(const game_type& gamestate) const override
    {
        std::vector<mkp::player_action_t> ret;

        if (const auto st = gamestate.gamestate_v();
            // st == mkp::gb_gamestate_t::PREFLOP_BET || st == mkp::gb_gamestate_t::FLOP_BET)
            st == mkp::gb_gamestate_t::PREFLOP_BET)
        {
            // XX%-sized raise: amount to call + XX * (amount to call + pot size) / 100
            int raise_200 = gamestate.amount_to_call() + 2 * (gamestate.amount_to_call() + gamestate.pot_size());
            int raise_100 = gamestate.amount_to_call() + 1 * (gamestate.amount_to_call() + gamestate.pot_size());
            int raise_50 = gamestate.amount_to_call() + 0.5 * (gamestate.amount_to_call() + gamestate.pot_size());

            const auto all = gamestate.possible_actions();
            std::copy_if(all.cbegin(), all.cend(), std::back_inserter(ret), [&](const mkp::player_action_t a) {
                if (a.m_action == mkp::gb_action_t::FOLD || a.m_action == mkp::gb_action_t::CALL || a.m_action == mkp::gb_action_t::CHECK ||
                    a.m_action == mkp::gb_action_t::ALLIN ||
                    (a.m_action == mkp::gb_action_t::RAISE && (std::abs(a.m_amount - raise_200) < 250 || a.m_amount - raise_200 == 250)) ||
                    (a.m_action == mkp::gb_action_t::RAISE && (std::abs(a.m_amount - raise_100) < 250 || a.m_amount - raise_100 == 250)) ||
                    (a.m_action == mkp::gb_action_t::RAISE && (std::abs(a.m_amount - raise_50) < 250 || a.m_amount - raise_50 == 250)))
                {
                    return true;
                }
                else
                {
                    return false;
                }
            });
        }
        else
        {
            ret.emplace_back(0, mkp::gb_action_t::CHECK, gamestate.active_player_v());
        }
        return ret;
    }
};

auto main() -> int
{
    fmt::print("start main\n");

    // simplified game: only preflop and flop actions are allowed with specific bet sizes
    using game_type = mkp::gamestate<c_num_players, 0, 1>;
    using cfrd_type = mkp::cfr_data<c_num_players, game_type, uint32_t>;
    game_type m_game_2p{100'000};
    mkp::gamestate_enumerator<game_type, uint32_t> m_enc_2p{};
    action_abstraction_pf_flop_only<game_type> m_aa_2p{};
    fmt::print("before ca\n");
    card_abstraction_norm_pf_flop_only<c_num_players, uint32_t> m_ca_2p{};
    fmt::print("after ca\n");

    decltype(mkp::init_tree(m_game_2p, &m_enc_2p, &m_aa_2p)) gametree_base_2p;
    std::unique_ptr<cfrd_type> m_cfrd;

    std::string m_jsonTree;
    bool m_tree_done = false;
    bool m_cfr_done = false;

    // std::mutex mu;
    // void printSynchronized(const std::string str)
    // {
    //     // make printing look prettier
    //     std::lock_guard<std::mutex> guard(mu);
    //     std::cout << str << std::endl;
    // }
    fmt::print("before init\n");

    // start calc (init) tree
    {
        if (!m_tree_done)
        {
            fmt::print("init tree\n");

            auto tmp = mkp::init_tree(m_game_2p, &m_enc_2p, &m_aa_2p);
            gametree_base_2p = std::move(tmp);
            // this->m_jsonTree = mkp::json_node(gametree_base_2p.get(), m_enc_2p).dump();
            m_tree_done = true;
            fmt::print("init tree done\n");
        }
    }

    // start cfr calc
    {
        if (!m_cfr_done)
        {
            fmt::print("starting cfr calc\n");
            m_cfrd = std::make_unique<cfrd_type>(std::move(gametree_base_2p), &m_enc_2p, &m_aa_2p, &m_ca_2p);

            std::vector<std::thread> workers;
            // for (int tid = 0; tid < (std::thread::hardware_concurrency() - 1); ++tid)
            for (int tid = 0; tid < 2; ++tid)
            {
                std::mutex mu;
                {
                    std::lock_guard<std::mutex> guard(mu);
                    fmt::print("starting workers (guarded)...\n");
                }
                fmt::print("starting workers...\n");
                workers.push_back(std::thread([cfrd_2p = m_cfrd.get(), tid]() {
                    mkp::card_generator cgen{};
                    std::array<std::array<int32_t, 2>, 65536> util{};
                    for (uint32_t i = 0; i < 3; ++i)
                    {
                        if (i % 10 == 0 || true)
                        {
                            const auto stats = mkp::regret_stats(cfrd_2p->m_root.get(), cfrd_2p->m_regret_sum);
                            const auto sum_util =
                                std::accumulate(util.cbegin(), util.cend(), int64_t(0),
                                                [](const int64_t lhs, const std::array<int32_t, 2>& rhs) { return lhs + rhs[0]; });
                            const auto sum_util_r =
                                std::accumulate(util.cbegin(), util.cend(), int64_t(0),
                                                [](const int64_t lhs, const std::array<int32_t, 2>& rhs) { return lhs + rhs[1]; });
                            const int64_t sz = util.size();

                            {
                                const auto str = fmt::format(
                                    "thread: {}\n stats after {} iterations | sum: {}, min: {}, max: "
                                    "{}\naverage "
                                    "utility: {} & {}",
                                    tid, i, stats.sum, stats.min, stats.max, sum_util / sz, sum_util_r / sz);

                                // std::mutex mu;
                                {
                                    // make printing look prettier
                                    // std::lock_guard<std::mutex> guard(mu);
                                    fmt::print(str);
                                }
                            }
                        }

                        const mkp::gamecards<2> cards(cgen.generate_v(9));
                        util[i % util.size()] = cfr_2p(cards, *cfrd_2p, cfrd_2p->m_root.get(), {1.0, 1.0});
                    }
                }));
            }
            fmt::print("trainig AI...\n");

            //std::for_each(workers.begin(), workers.end(), [](std::thread& t) { t.detach(); });

            m_cfr_done = true;
        }
    }

    // Napi::Value getTree(const Napi::CallbackInfo& info)
    // {
    //     Napi::Env env = info.Env();
    //     if (m_tree_done)
    //     {
    //         return Napi::String::New(env, this->m_jsonTree);
    //     }
    //     else
    //     {
    //         return Napi::String::New(env, "");
    //     }
    // }

    // Napi::Value getActions(const Napi::CallbackInfo& info)
    // {
    //     Napi::Env env = info.Env();

    //     if (m_tree_done)
    //     {
    //         const auto id = info[0].As<Napi::Number>().Uint32Value();

    //         const auto game = m_enc_2p.decode(id);
    //         const auto actions = m_aa_2p.filter_actions(game);

    //         auto gameActions = nlohmann::json::array();
    //         for (auto i = 0; i < actions.size(); ++i)
    //         {
    //             gameActions.push_back(nlohmann::json{
    //                 {"actionId", i}, {"actionName", mkp::to_string(actions[i].m_action)}, {"actionAmount", actions[i].m_amount}});
    //         }

    //         return Napi::String::New(env, gameActions.dump());
    //     }
    //     else
    //     {
    //         return Napi::String::New(env, "");
    //     }
    // }

    // Napi::Value getGame(const Napi::CallbackInfo& info)
    // {
    //     Napi::Env env = info.Env();

    //     if (m_tree_done)
    //     {
    //         const auto id = info[0].As<Napi::Number>().Uint32Value();

    //         const auto game = m_enc_2p.decode(id);
    //         const auto cf = game.chips_front();
    //         const auto cb = game.chips_behind();
    //         const auto st = game.all_players_state();

    //         auto gamePlayers = nlohmann::json::array();
    //         for (auto i = 0; i < c_num_players; ++i)
    //         {
    //             gamePlayers.push_back(nlohmann::json{{"playerChips", cb[i]}, {"playerBet", cf[i]}, {"playerStatus", st[i]}});
    //         }

    //         return Napi::String::New(env, nlohmann::json{{"gameId", id},
    //                                                      {"gameInfo", game.str_state()},
    //                                                      {"gameStatus", static_cast<int>(game.gamestate_v())},
    //                                                      {"gameStatusStr", mkp::to_string(game.gamestate_v())},
    //                                                      {"gameCurrentPlayer", game.active_player()},
    //                                                      {"gameMinRaise", game.minraise()},
    //                                                      {"gamePlayers", gamePlayers}}
    //                                           .dump());
    //     }
    //     else
    //     {
    //         return Napi::String::New(env, "");
    //     }
    // }

    // Napi::Value getCfrData(const Napi::CallbackInfo& info)
    // {
    //     Napi::Env env = info.Env();

    //     if (m_tree_done)
    //     {
    //         const auto gs_id = info[0].As<Napi::Number>().Uint32Value();
    //         const auto gs = m_enc_2p.decode(gs_id);
    //         const auto all_actions = m_aa_2p.filter_actions(gs);

    //         const std::vector<std::pair<uint32_t, float>> vec_init;
    //         std::vector<std::vector<std::pair<uint32_t, float>>> vec_temp(all_actions.size(), vec_init);
    //         for (uint32_t i = 0; i < m_cfrd->m_strategy_sum[gs_id].size(); ++i)
    //         {
    //             // skip empty indices
    //             if (std::reduce(m_cfrd->m_strategy_sum[gs_id][i].cbegin(), m_cfrd->m_strategy_sum[gs_id][i].cend()) == 0)
    //             {
    //                 continue;
    //             }

    //             const auto values = mkp::normalize(m_cfrd->m_strategy_sum[gs_id][i]);
    //             for (uint32_t j = 0; j < values.size(); ++j)
    //             {
    //                 vec_temp[j].emplace_back(i, values[j]);
    //             }
    //         }

    //         auto allActions = nlohmann::json::array();
    //         for (uint32_t i = 0; i < vec_temp.size(); ++i)
    //         {
    //             auto allValues = nlohmann::json::array();
    //             for (uint32_t j = 0; j < vec_temp[i].size(); ++j)
    //             {
    //                 nlohmann::json pokerActionValue{{"description", m_ca_2p.str_id(gs.gamestate_v(), vec_temp[i][j].first)},
    //                                                 {"value", vec_temp[i][j].second},
    //                                                 {"bucket", m_ca_2p.str_id_json(gs.gamestate_v(), vec_temp[i][j].first)}};
    //                 allValues.push_back(pokerActionValue);
    //             }
    //             nlohmann::json pokerAction{{"actionDescription", all_actions[i].str()}, {"actionContent", allValues}};
    //             allActions.push_back(pokerAction);
    //         }
    //         return Napi::String::New(env, allActions.dump());
    //     }
    //     else
    //     {
    //         return Napi::String::New(env, "");
    //     }
    // }
}
