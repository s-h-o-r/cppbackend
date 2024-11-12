#include <catch2/catch_test_macros.hpp>

#include "../src/htmldecode.h"

using namespace std::literals;

TEST_CASE("Mnemonics test", "[HtmlDecode]") {
    CHECK(HtmlDecode(""sv) == ""s);
    CHECK(HtmlDecode("hello"sv) == "hello"s);
    CHECK(HtmlDecode("Some other text"sv) == "Some other text"s);
    CHECK(HtmlDecode("Johnson&amp;Johnson"sv) == "Johnson&Johnson"s);
    CHECK(HtmlDecode("Johnson&ampJohnson"sv) == "Johnson&Johnson"s);
    CHECK(HtmlDecode("Johnson&AMPJohnson"sv) == "Johnson&Johnson"s);
    CHECK(HtmlDecode("Johnson&AMP;Johnson"sv) == "Johnson&Johnson"s);
    CHECK(HtmlDecode("Johnson&Johnson"sv) == "Johnson&Johnson"s);
    CHECK(HtmlDecode("Johnson&aMP;Johnson"sv) == "Johnson&aMP;Johnson"s);
    CHECK(HtmlDecode("&lt&gt&AMP;&apos;&QUOT"sv) == "<>&'\""s);
    CHECK(HtmlDecode("&lt&gt&AM;&aps;&QUO"sv) == "<>&AM;&aps;&QUO"s);
    CHECK(HtmlDecode("&lt"sv) == "<"s);
    CHECK(HtmlDecode("&lt;"sv) == "<"s);
    CHECK(HtmlDecode("&LT;"sv) == "<"s);
    CHECK(HtmlDecode("&LT"sv) == "<"s);
    CHECK(HtmlDecode("&amp;lt;"sv) == "&lt;"s);
}
