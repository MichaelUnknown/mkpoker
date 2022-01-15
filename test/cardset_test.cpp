#include <mkpoker/base/cardset.hpp>

#include <algorithm>    // std::sort, std::find
#include <array>
#include <bitset>
#include <random>
#include <stdexcept>
#include <vector>

#include <gtest/gtest.h>

using namespace mkp;

TEST(tcardset, cardset_ctor_bitset)
{
    const cardset empty{};
    const cardset some{0b1100'0000'0101'1111'0101'1000};
    const cardset all{c_cardset_full};

    EXPECT_EQ(empty.size(), 0);
    EXPECT_EQ(some.size(), 11);
    EXPECT_EQ(all.size(), 52);
    EXPECT_THROW(cardset{0xFFFF'FFFF'FFFF'FFFF}, std::runtime_error);
    EXPECT_THROW(cardset{0xFFFF'FFFF'FFFF'FFFF << c_deck_size}, std::runtime_error);
    EXPECT_THROW(cardset{uint64_t(1) << c_deck_size}, std::runtime_error);
    EXPECT_THROW(cardset{c_cardset_full + 1}, std::runtime_error);
}

TEST(tcardset, cardset_ctor_string)
{
    EXPECT_EQ("Ac", cardset("Ac").str());
    EXPECT_EQ(cardset("Ac").size(), 1);
    EXPECT_EQ(cardset("Ac"), cardset("ac"));

    EXPECT_EQ("AcAs", cardset("AcAs").str());
    EXPECT_EQ(cardset("AcAs").size(), 2);
    EXPECT_EQ(cardset("ksqc"), cardset("KSQC"));
    EXPECT_EQ(cardset("ksqc"), cardset("KsQc"));
    EXPECT_EQ("2dQh", cardset("2dQh").str());
    EXPECT_EQ("2dQh", cardset("qh2d").str());

    EXPECT_EQ("2dQhAs", cardset("Asqh2d").str());
    EXPECT_EQ(cardset("2dQhAs").size(), 3);

    EXPECT_EQ(cardset("AsAsAsAsasas").size(), 1);
    EXPECT_EQ(cardset("AsKsQhJdTc9c2c").size(), 7);
    EXPECT_EQ(cardset("2c3c4c5c6c7c8c9cTcJcQcKcAc"
                      "2d3d4d5d6d7d8d9dTdJdQdKdAd"
                      "2h3h4h5h6h7h8h9hThJhQhKhAh"
                      "2s3s4s5s6s7s8s9sTsJsQsKsAs")
                  .size(),
              52);

    EXPECT_THROW(cardset("A"), std::runtime_error);
    EXPECT_THROW(cardset("AcA"), std::runtime_error);
    EXPECT_THROW(cardset("AcAA"), std::runtime_error);
    EXPECT_THROW(cardset("KKKs"), std::runtime_error);
    EXPECT_THROW(cardset("hJ"), std::runtime_error);
    EXPECT_THROW(cardset("99"), std::runtime_error);
}

TEST(tcardset, cardset_ctor_container)
{
    const std::array<card, 3> c_arr{card{1}, card{7}, card{42}};
    const auto cs_a = cardset(c_arr);
    const std::vector<card> c_v(c_arr.begin(), c_arr.end());
    const auto cs_v = cardset(c_v);

    EXPECT_EQ(3, cs_a.size());
    EXPECT_EQ(3, cs_v.size());
    EXPECT_EQ(cs_a, cs_v);

    EXPECT_EQ(c_v, cs_a.as_cards());
    EXPECT_EQ(c_v, cs_v.as_cards());
}

TEST(tcardset, cardset_fill_clear)
{
    cardset all{};
    all.fill();
    EXPECT_EQ(all.size(), 52);
    all.clear();
    EXPECT_EQ(all.size(), 0);
}

TEST(tcardset, cardset_insert_remove_contains)
{
    cardset cs{};
    for (uint8_t i = 0; i < c_deck_size; ++i)
    {
        card c{i};
        cs.insert(c);
        EXPECT_EQ(cs.size(), (i + 1));
        EXPECT_EQ(cs.contains(c), true);

        auto cs2 = cs;
        cs2.remove(c);
        EXPECT_EQ(cs2.size(), (cs.size() - 1));
        EXPECT_EQ(cs2.contains(c), false);

        EXPECT_EQ(cs.contains(cs), true);
        EXPECT_EQ(cs2.contains(cs2), true);
        EXPECT_EQ(cs.contains(cs2), true);
        EXPECT_EQ(cs2.contains(cs), false);
    }
}

TEST(tcardset, cardset_str)
{
    // 10000 random tests: generate cardset from random strings and compare str output with original

    std::mt19937 rng(std::random_device{}());

    const auto all_cards = []() {
        std::vector<uint8_t> cards{};
        cards.reserve(c_deck_size);
        for (uint8_t i = 0; i < c_deck_size; ++i)
        {
            cards.emplace_back(static_cast<uint8_t>(i));
        }
        return cards;
    }();

    for (unsigned i = 0; i < 10000; ++i)
    {
        // take 5 random unique cards
        auto cards = all_cards;
        std::shuffle(cards.begin(), cards.end(), rng);
        cards.resize(5);

        // create cardset from sorted vector of numbers
        // cardset str() will always output cards canonically sorted
        {
            std::sort(cards.begin(), cards.end());
            const auto str_cards = [&]() {
                std::string str{};
                for (auto&& c : cards)
                {
                    str.append(card{c}.str());
                }
                return str;
            }();
            const cardset cs{str_cards};
            EXPECT_EQ(cs.str(), str_cards);
        }

        // create cardset from unsorted (random) vector
        // check if every card from str is in cardset output
        {
            const auto str_cards_unsorted = [&]() {
                std::string str{};
                for (auto&& c : cards)
                {
                    str.append(card{c}.str());
                }
                return str;
            }();
            const cardset cs{str_cards_unsorted};
            const auto str_cs = cs.str();

            for (unsigned j = 0; j < str_cards_unsorted.size(); ++j)
            {
                const auto str_card = str_cards_unsorted.substr(j, j + 2);
                // str_cs must contain every card (string)
                EXPECT_NE(str_cs.find(str_card), std::string::npos);
            }
        }
    }
}

TEST(tcardset, cardset_combine)
{
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<uint64_t> dist(1, c_cardset_full);

    for (auto i = 0; i < 10000;)
    {
        try
        {
            const cardset cs(dist(rng));
            const auto cards = cs.as_cards();
            cardset cs2 = cs;
            cardset cs3{};
            for (auto&& c : cards)
            {
                cs2 = cs2.combine(c);
                cs3 = cs3.combine(c);
            }
            EXPECT_EQ(cs, cs2);
            EXPECT_EQ(cs, cs3);
            EXPECT_EQ(cs.str(), cs2.str());
            ++i;
        }
        catch (...)
        {
        }
    }
}
