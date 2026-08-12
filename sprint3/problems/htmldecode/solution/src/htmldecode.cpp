#include "htmldecode.h"

#include <string>
#include <string_view>
#include <vector>

struct EntityMapping {
    std::string_view lowerName;
    std::string_view upperName;
    char replacement;
};

static constexpr EntityMapping ENTITIES[] = {
    {"lt",   "LT",   '<'},
    {"gt",   "GT",   '>'},
    {"amp",  "AMP",  '&'},
    {"apos", "APOS", '\''},
    {"quot", "QUOT", '"'}
};

std::string HtmlDecode(std::string_view str) {
    std::string result;
    result.reserve(str.size());

    size_t i = 0;
    const size_t n = str.size();

    while (i < n) {
        if (str[i] != '&') {
            result.push_back(str[i]);
            ++i;
            continue;
        }

        bool matched = false;

        for (const auto& entity : ENTITIES) {
            if (str.substr(i + 1).starts_with(entity.lowerName)) {
                size_t matchedLen = 1 + entity.lowerName.size();
                if (i + matchedLen < n && str[i + matchedLen] == ';') {
                    ++matchedLen;
                }
                result.push_back(entity.replacement);
                i += matchedLen;
                matched = true;
                break;
            }
            if (str.substr(i + 1).starts_with(entity.upperName)) {
                size_t matchedLen = 1 + entity.upperName.size();
                if (i + matchedLen < n && str[i + matchedLen] == ';') {
                    ++matchedLen;
                }
                result.push_back(entity.replacement);
                i += matchedLen;
                matched = true;
                break;
            }
        }

        if (!matched) {
            result.push_back(str[i]);
            ++i;
        }
    }

    return result;
}