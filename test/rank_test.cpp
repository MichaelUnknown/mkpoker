#include <mkpoker/base/rank.hpp>

#include <stdexcept>
#include <string>
#include <unordered_map>

#include <gtest/gtest.h>

using namespace mkpoker::base;

TEST(rank, rank_ctor_char)
{
    std::unordered_map<char, uint8_t> valid_input;
    valid_input['2'] = 0;
    valid_input['3'] = 1;
    valid_input['4'] = 2;
    valid_input['5'] = 3;
    valid_input['6'] = 4;
    valid_input['7'] = 5;
    valid_input['8'] = 6;
    valid_input['9'] = 7;
    valid_input['t'] = 8;
    valid_input['T'] = 8;
    valid_input['j'] = 9;
    valid_input['J'] = 9;
    valid_input['q'] = 10;
    valid_input['Q'] = 10;
    valid_input['k'] = 11;
    valid_input['K'] = 11;
    valid_input['a'] = 12;
    valid_input['A'] = 12;

    // test the char range
    for (uint8_t i = 0; i < 255; ++i)
    {
        if (const auto c = static_cast<char>(i); valid_input.contains(c))
        {
            EXPECT_EQ(valid_input.at(c), rank{c}.m_rank);
            EXPECT_EQ(std::string(1, static_cast<char>(std::toupper(c))), rank{c}.str());
        }
        else
        {
            EXPECT_THROW(rank{c}, std::runtime_error);
        }
    }
}

TEST(rank, rank_ctor_int)
{
    // test the valid unsigned int range
    for (uint8_t i = 0; i <= 12; ++i)
    {
        rank r{rank_t{i}};
        EXPECT_EQ(i, r.m_rank);
    }
    // test the valid int range
    for (int i = 0; i <= 12; ++i)
    {
        rank r{rank_t{uint8_t(i)}};
        EXPECT_EQ(i, r.m_rank);
    }

    // some invalid range
    for (uint8_t i = 13; i < 255; ++i)
    {
        EXPECT_THROW(rank{rank_t{i}}, std::runtime_error);
    }
}

TEST(rank, rank_ctor_string)
{
    std::unordered_map<std::string, uint8_t> valid_input;
    valid_input["2"] = 0;
    valid_input["3"] = 1;
    valid_input["4"] = 2;
    valid_input["5"] = 3;
    valid_input["6"] = 4;
    valid_input["7"] = 5;
    valid_input["8"] = 6;
    valid_input["9"] = 7;
    valid_input["t"] = 8;
    valid_input["T"] = 8;
    valid_input["j"] = 9;
    valid_input["J"] = 9;
    valid_input["q"] = 10;
    valid_input["Q"] = 10;
    valid_input["k"] = 11;
    valid_input["K"] = 11;
    valid_input["a"] = 12;
    valid_input["A"] = 12;

    // test the char range, but convert to string
    for (unsigned char c = 0; c < 255; ++c)
    {
        if (const auto s = std::string(1, c); valid_input.contains(s))
        {
            EXPECT_EQ(valid_input.at(s), rank{s}.m_rank);
            EXPECT_EQ(std::string(1, static_cast<char>(std::toupper(c))), rank{s}.str());
        }
        else
        {
            EXPECT_THROW(rank{s}, std::runtime_error);
        }
    }

#ifndef _DEBUG
    // these will trigger an assert in msvc/debug
    EXPECT_THROW(rank{""}, std::runtime_error);
#endif
    EXPECT_THROW(rank{"too long"}, std::runtime_error);
}

TEST(rank, rank_comparison_operators)
{
    EXPECT_TRUE(rank{rank_t::three} > rank{rank_t::two});
    EXPECT_TRUE(rank{rank_t::seven} < rank{rank_t::jack});
    EXPECT_TRUE(rank{rank_t::queen} == rank{rank_t::queen});
    EXPECT_TRUE(rank{rank_t::ten} != rank{rank_t::king});
    EXPECT_TRUE(rank{rank_t::five} >= rank{rank_t::five});
    EXPECT_TRUE(rank{rank_t::nine} <= rank{rank_t::ten});
}
