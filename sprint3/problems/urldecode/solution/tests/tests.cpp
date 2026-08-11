#define BOOST_TEST_MODULE urlencode tests
#include <boost/test/unit_test.hpp>

#include "../src/urldecode.h"

BOOST_AUTO_TEST_CASE(UrlDecode_tests) {
    using namespace std::literals;

    // Пустая строка.
    BOOST_TEST(UrlDecode(""sv) == ""s);
    // Строка без %-последовательностей.
    BOOST_TEST(UrlDecode("https://store.steampowered.com/app/1808500/ARC_Raiders/"sv) == "https://store.steampowered.com/app/1808500/ARC_Raiders/"s);
    BOOST_TEST(UrlDecode("https://shared.akamai.steamstatic.com/store_item_assets/steam/apps/1808500/library_hero_2x.jpg"sv) == "https://shared.akamai.steamstatic.com/store_item_assets/steam/apps/1808500/library_hero_2x.jpg"s);
    // Строка с валидными %-последовательностями, записанными в разном регистре.
    BOOST_TEST(UrlDecode("https://store.steampowered.com/search/?term=cyberpunk%202077&category1=998%2f997"sv) == "https://store.steampowered.com/search/?term=cyberpunk 2077&category1=998/997"s);
    // Строка с невалидными %-последовательностями.
    BOOST_CHECK_EXCEPTION(UrlDecode("https://shared.fastly.steamstatic.com/store_item_assets/steam/apps/1091500/ss_4eb068b1cf52c91b57157b84bed18a186ed7714b.1920x1080.jpg?t%3G1784714077"sv),
                          std::invalid_argument,
                          [](const std::invalid_argument& e) {
                              return std::string(e.what()) == "Invalid hex sequence";
                          });
    // Строка с неполными %-последовательностями.
    BOOST_CHECK_EXCEPTION(UrlDecode("https://store.steampowered.com/app/2073850/THE_FINALS%A"sv),
                          std::invalid_argument,
                          [](const std::invalid_argument& e) {
                              return std::string(e.what()) == "Incomplete sequence";
                          });
    // Строка с символом +.
    BOOST_TEST(UrlDecode("Hello+World"sv) == "Hello World"s);
}