#include <mkpoker/base/suit.hpp>

#include <stdexcept>
#include <string>
#include <unordered_map>

#include <gtest/gtest.h>

using namespace mkpoker::base;

TEST(suit, suit_ctor_char)
{
    std::unordered_map<char, uint8_t> valid_input;
    valid_input['c'] = 0;
    valid_input['d'] = 1;
    valid_input['h'] = 2;
    valid_input['s'] = 3;
    valid_input['C'] = 0;
    valid_input['D'] = 1;
    valid_input['H'] = 2;
    valid_input['S'] = 3;

    // test the char range
    for (uint8_t i = 0; i < 255; ++i)
    {
        if (const auto c = static_cast<char>(i); valid_input.contains(c))
        {
            EXPECT_EQ(valid_input.at(c), suit{c}.m_suit);
            EXPECT_EQ(std::string(1, static_cast<char>(std::tolower(c))), suit{c}.str());
        }
        else
        {
            EXPECT_THROW(suit{c}, std::runtime_error);
        }
    }
}

TEST(suit, suit_ctor_int)
{
    // test the valid unsigned int range
    for (uint8_t i = 0; i <= 3; ++i)
    {
        suit s{suit_t{i}};
        EXPECT_EQ(i, s.m_suit);
    }
    // test the valid int range
    for (int i = 0; i <= 3; ++i)
    {
        suit s{suit_t{i}};
        EXPECT_EQ(i, s.m_suit);
    }

    // some invalid range
    for (uint8_t i = 4; i < 255; ++i)
    {
        EXPECT_THROW(suit{suit_t{i}}, std::runtime_error);
    }
}

TEST(suit, suit_ctor_string)
{
    std::unordered_map<std::string, uint8_t> valid_input;
    valid_input["c"] = 0;
    valid_input["d"] = 1;
    valid_input["h"] = 2;
    valid_input["s"] = 3;
    valid_input["C"] = 0;
    valid_input["D"] = 1;
    valid_input["H"] = 2;
    valid_input["S"] = 3;

    // test the char range, but convert to string
    for (unsigned char c = 0; c < 255; ++c)
    {
        if (const auto s = std::string(1, c); valid_input.contains(s))
        {
            EXPECT_EQ(valid_input.at(s), suit{s}.m_suit);
            EXPECT_EQ(std::string(1, static_cast<char>(std::tolower(c))), suit{s}.str());
        }
        else
        {
            EXPECT_THROW(suit{s}, std::runtime_error);
        }
    }
}

TEST(suit, suit_comparison_operators)
{
    EXPECT_TRUE(suit{suit_t::diamonds} > suit{suit_t::clubs});
    EXPECT_TRUE(suit{suit_t::diamonds} < suit{suit_t::spades});
    EXPECT_TRUE(suit{suit_t::hearts} == suit{suit_t::hearts});
    EXPECT_TRUE(suit{suit_t::hearts} != suit{suit_t::spades});
    EXPECT_TRUE(suit{suit_t::diamonds} >= suit{suit_t::diamonds});
    EXPECT_TRUE(suit{suit_t::clubs} <= suit{suit_t::clubs});
}
