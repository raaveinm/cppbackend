#pragma once
#include <iomanip>
#include <random>
#include <sstream>
#include <string>

namespace util {

class TokenGenerator {
public:
    TokenGenerator() = default;

    std::string GenerateToken() {
        const uint64_t num1 = generator1_();
        const uint64_t num2 = generator2_();

        std::ostringstream ss;
        ss << std::hex << std::setfill('0')
           << std::setw(16) << num1
           << std::setw(16) << num2;

        return ss.str();
    }

private:
    std::mt19937_64 generator1_{std::random_device{}()};
    std::mt19937_64 generator2_{std::random_device{}()};
};

}  // namespace util