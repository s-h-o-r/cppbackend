#define BOOST_TEST_MODULE urlencode tests
#include <boost/test/unit_test.hpp>

#include "../src/urldecode.h"

BOOST_AUTO_TEST_CASE(UrlDecode_tests) {
    using namespace std::literals;

    BOOST_TEST(UrlDecode(""sv) == ""s);
    BOOST_TEST(UrlDecode("Hello World!"sv) == "Hello World!"s);
    BOOST_TEST(UrlDecode("%2a%2A %2f%2F"sv) == "** //"s);
    BOOST_TEST(UrlDecode("+"sv) == " "s);
    BOOST_TEST(UrlDecode("Hello+World!"sv) == "Hello World!"s);

    //некорректная последовательность
    BOOST_CHECK_THROW(UrlDecode("%2Z"sv), std::invalid_argument);
    //неполная последовательность
    BOOST_CHECK_THROW(UrlDecode("%2 hello"sv), std::invalid_argument);

    BOOST_CHECK_THROW(UrlDecode("%2"sv), std::invalid_argument);
}
