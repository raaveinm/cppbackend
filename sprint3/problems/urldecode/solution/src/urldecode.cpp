#include "urldecode.h"

#include <stdexcept>
#include <string>
#include <string_view>

int HexToNum(const char ch) {
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    return -1;
}

std::string UrlDecode(const std::string_view str) {
    std::string result;
    result.reserve(str.size());

    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] == '+') {
            result.push_back(' ');
        } else if (str[i] == '%') {
            if (i + 2 >= str.size())
                throw std::invalid_argument("Incomplete sequence");

            const int high = HexToNum(str[i + 1]);
            const int low = HexToNum(str[i + 2]);

            if (high == -1 || low == -1)
                throw std::invalid_argument("Invalid hex sequence");

            result.push_back(static_cast<char>(high << 4 | low));
            i += 2;
        } else {
            result.push_back(str[i]);
        }
    }
    return result;
}