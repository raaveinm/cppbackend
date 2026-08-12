#include "../src/urlencode.h"
#include <gmock/gmock.h>
#include <string>

using namespace std::string_view_literals;
using namespace ::testing;

TEST(UrlEncode, EncodesDollar) {
    ASSERT_EQ(UrlEncode("a$b"sv), "a%24b");
}

TEST(UrlEncode, EncodesAmperstand) {
    ASSERT_EQ(UrlEncode("a&b"sv), "a%26b");
}

TEST(UrlEncode, EncodesRussianChars) {
    ASSERT_THAT(UrlEncode("Привет"sv), MatchesRegex("%(d0|D0)%(9f|9F)%(d1|D1)%(80|80)%(d0|D0)%(b8|B8)%(d0|D0)%(b2|B2)%(d0|D0)%(b5|B5)%(d1|D1)%(82|82)"));
}

TEST(UrlEncode, DoesNotEncodeUnreserved) {
    ASSERT_EQ(UrlEncode("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_.~"sv), "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_.~");
}

TEST(UrlEncode, EncodesEmpty) {
    ASSERT_EQ(UrlEncode(""sv), "");
}

TEST(UrlEncode, EncodesMixed) {
    ASSERT_EQ(UrlEncode("a b_c-d~e.f$g"sv), "a+b_c-d~e.f%24g");
}
