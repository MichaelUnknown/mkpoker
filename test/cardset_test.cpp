#include <mkpoker/base/cardset.hpp>

#include <array>
#include <random>
#include <stdexcept>
#include <vector>

#include <gtest/gtest.h>

using namespace mkp;

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
    cardset all;
    all.fill();
    EXPECT_EQ(all.size(), 52);
    all.clear();
    EXPECT_EQ(all.size(), 0);
}

TEST(tcardset, cardset_str)
{
    const cardset cs("2cThKs6d3c");
    EXPECT_EQ(cs.str(), std::string("2c3c6dThKs"));
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
