//
// Created by raaveinm on 8/6/26.
//

#include <chrono>
#include <iosfwd>
#include <iostream>
#include <sstream>

class LogDuration {
public:
    explicit LogDuration(std::string_view msg = "")
      : message(std::string(msg) + ": ")
      , start(std::chrono::steady_clock::now())
    {
    }

    ~LogDuration() {
        const auto finish = std::chrono::steady_clock::now();
        const auto dur = finish - start;
        std::ostringstream os;
        os << message
           << std::chrono::duration_cast<std::chrono::milliseconds>(dur).count()
           << " ms" << std::endl;
        std::cerr << os.str();
    }
private:
    std::string message;
    std::chrono::steady_clock::time_point start;
};
