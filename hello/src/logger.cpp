//
// Created by raaveinm on 7/12/26.
//

#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <syncstream>
#include <string>

class Logger {
    static auto GetTimeStamp() {
        const auto time_point = std::chrono::system_clock::now();
        const auto time_t_time = std::chrono::system_clock::to_time_t(time_point);
        return std::put_time(std::gmtime(&time_t_time), "%Y-%m-%d %H:%M:%S");
    }

public:
    Logger() = default;

    template<typename T>
    void Log(const T& message) {
        std::osyncstream sync_stream(log_file_);
        sync_stream << GetTimeStamp() << ": ";
        sync_stream << message;
        sync_stream << std::endl;
    }

private:
    std::ofstream log_file_{"sample.txt"};
};

int main() {
    Logger log;
    log.Log("Program started");

    int x, y;
    std::cout << "Enter x and y: ";
    std::cin >> x >> y;
    log.Log("User entered numbers " + std::to_string(x) + " and " + std::to_string(y));

    int sum = x + y;
    std::cout << "The sum of x and y is " << sum << std::endl;

    log.Log("The sum " + std::to_string(sum) + " is computed");
    return 0;
};