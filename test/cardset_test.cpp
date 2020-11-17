#include <mkpoker/base/cardset.hpp>

#include <array>
#include <stdexcept>
#include <vector>

#include <gtest/gtest.h>

using namespace mkpoker::base;

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

//TEST(cardset, cardset_rotate_suit)
//{
//    const cardset cs1("2c6dThKs");
//    const cardset cs2("2d6hTsKc");
//    const cardset cs3("2h6sTcKd");
//    const cardset cs4("2s6cTdKh");
//    EXPECT_EQ(cs1.rotate_suits(0, 1, 2, 3), cs1);
//    EXPECT_EQ(cs1.rotate_suits(1, 2, 3, 0), cs2);
//    EXPECT_EQ(cs1.rotate_suits(2, 3, 0, 1), cs3);
//    EXPECT_EQ(cs1.rotate_suits(3, 0, 1, 2), cs4);
//    EXPECT_EQ(cs2.rotate_suits(3, 0, 1, 2), cs1);
//    EXPECT_THROW(cs1.rotate_suits(0, 1, 2, -1), std::runtime_error);
//}
