#include <catch2/catch_test_macros.hpp>

#include "../src/htmldecode.h"

using namespace std::literals;

TEST_CASE("Text without mnemonics", "[HtmlDecode]") {
    CHECK(HtmlDecode(""sv) == ""s);
    CHECK(HtmlDecode("hello"sv) == "hello"s);
}

TEST_CASE("HtmlDecode correctly handles basic entities", "[HtmlDecode]") {
    SECTION("Empty string") {
        REQUIRE(HtmlDecode("") == "");
    }

    SECTION("String without entities") {
        REQUIRE(HtmlDecode("Hello, World!") == "Hello, World!");
    }

    SECTION("Standard lowercase entities with semicolons") {
        REQUIRE(HtmlDecode("&lt;&gt;&amp;&apos;&quot;") == "<>&'\"");
    }

    SECTION("Standard uppercase entities with semicolons") {
        REQUIRE(HtmlDecode("&LT;&GT;&AMP;&APOS;&QUOT;") == "<>&'\"");
    }

    SECTION("Entities without trailing semicolons") {
        REQUIRE(HtmlDecode("&lt&gt&amp&apos&quot") == "<>&'\"");
    }
}

TEST_CASE("HtmlDecode validates casing strictness", "[HtmlDecode]") {
    SECTION("Mixed case in body is invalid and left untouched") {
        REQUIRE(HtmlDecode("&aPos;") == "&aPos;");
        REQUIRE(HtmlDecode("&aPOS;") == "&aPOS;");
        REQUIRE(HtmlDecode("&Apos;") == "&Apos;");
        REQUIRE(HtmlDecode("&AmP") == "&AmP");
    }

    SECTION("Valid uniform case combinations in continuous text") {
        REQUIRE(HtmlDecode("M&amp;M&APOSs") == "M&M's");
        REQUIRE(HtmlDecode("Johnson&amp;Johnson") == "Johnson&Johnson");
        REQUIRE(HtmlDecode("Johnson&AMPJohnson") == "Johnson&Johnson");
    }
}

TEST_CASE("HtmlDecode prevents recursive decoding and handles unknown entities", "[HtmlDecode]") {
    SECTION("Decoded symbols are not re-evaluated") {
        REQUIRE(HtmlDecode("&amp;lt;") == "&lt;");
        REQUIRE(HtmlDecode("&amp;amp;") == "&amp;");
    }

    SECTION("Unknown entity-like patterns are left intact") {
        REQUIRE(HtmlDecode("&abracadabra") == "&abracadabra");
        REQUIRE(HtmlDecode("&123;") == "&123;");
        REQUIRE(HtmlDecode("&&amp;") == "&&");
    }

    SECTION("Partial matches or dangling ampersands") {
        REQUIRE(HtmlDecode("&") == "&");
        REQUIRE(HtmlDecode("&l") == "&l");
        REQUIRE(HtmlDecode("&apositive") == "'itive");
    }
}

// Напишите недостающие тесты самостоятельно