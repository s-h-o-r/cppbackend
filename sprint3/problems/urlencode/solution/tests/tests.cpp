#include <gtest/gtest.h>

#include "../src/urlencode.h"

using namespace std::literals;

TEST(UrlEncodeTestSuite, OrdinaryCharsAreNotEncoded) {
    EXPECT_EQ(UrlEncode(""sv), ""s);
    EXPECT_EQ(UrlEncode("hello"sv), "hello"s);
    EXPECT_EQ(UrlEncode("hi~-_."sv), "hi~-_."s);
    EXPECT_EQ(UrlEncode("H e l l o"sv), "H+e+l+l+o"s);
    EXPECT_EQ(UrlEncode("!#$&'()*+,/:;=?@[]"sv), "%21%23%24%26%27%28%29%2a%2b%2c%2f%3a%3b%3d%3f%40%5b%5d"s);
    EXPECT_EQ(UrlEncode("! #b$ c &'()*+,/:;=?@[]"sv), "%21+%23b%24+c+%26%27%28%29%2a%2b%2c%2f%3a%3b%3d%3f%40%5b%5d"s);
    EXPECT_EQ(UrlEncode(""sv), ""s);
    EXPECT_EQ(UrlEncode(" "sv), "+"s);
    EXPECT_EQ(UrlEncode(" ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz-_.~"sv), "+ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz-_.~"s);
}

/* Напишите остальные тесты самостоятельно */
