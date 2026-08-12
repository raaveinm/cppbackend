#include "urlencode.h"

#include <string>
#include <string_view>
#include <iomanip>

std::string UrlEncode(std::string_view str) {
    std::string encoded;
    encoded.reserve(str.size());

    for (unsigned char c : str) {
        if (c == ' ') {
            encoded += '+';
            continue;
        }

        if (c < 32 || c >= 128) {
            encoded += '%';
            encoded += "0123456789ABCDEF"[c >> 4];
            encoded += "0123456789ABCDEF"[c & 0x0F];
            continue;
        }

        switch (c) {
            case '!': case '#': case '$': case '&': case '\'':
            case '(': case ')': case '*': case '+': case ',':
            case '/': case ':': case ';': case '=': case '?':
            case '@': case '[': case ']':
                encoded += '%';
                encoded += "0123456789ABCDEF"[c >> 4];
                encoded += "0123456789ABCDEF"[c & 0x0F];
                break;

            default:
                encoded += c;
                break;
        }
    }

    return encoded;
}