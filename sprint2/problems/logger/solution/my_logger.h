#pragma once

#include <chrono>
#include <iomanip>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <optional>
#include <mutex>
#include <thread>

using namespace std::literals;

#define LOG(...) Logger::GetInstance().Log(__VA_ARGS__)

class Logger {
    auto GetTime() const {
        if (manual_ts_) {
            return *manual_ts_;
        }

        return std::chrono::system_clock::now();
    }

    std::string GetTimeStamp() const {
        const auto now = GetTime();
        const auto t_c = std::chrono::system_clock::to_time_t(now);
        const std::tm* local_time = std::localtime(&t_c);
        std::stringstream ss;
        ss << std::put_time(local_time, "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }

    [[nodiscard]] std::string GetFileTimeStamp() const {
        const std::chrono::time_point<std::chrono::system_clock> now = GetTime();
        const std::time_t t_c = std::chrono::system_clock::to_time_t(now);
        const std::tm* local_time = std::localtime(&t_c);
        std::stringstream ss;
        ss << std::put_time(local_time, "%Y_%m_%d");
        return ss.str();
    }

    Logger() = default;
    Logger(const Logger&) = delete;

public:
    static Logger& GetInstance() {
        static Logger obj;
        return obj;
    }

    // Выведите в поток все аргументы.
    template<class... Ts>
    void Log(const Ts&... args) {
        std::lock_guard lock(mutex_);
        const std::string path = "/var/log/sample_log_" + GetFileTimeStamp() + ".log";
        try {
            std::ofstream file(path, std::ios::app);
            if (!file.is_open()) throw std::runtime_error("Could not open file.");
            file << GetTimeStamp() << ": ";
            (file << ... << args);
            file << "\n";

        } catch (const std::exception &e) {
            std::cerr << e.what() << std::endl;
        }
    }

    void SetTimestamp(std::chrono::system_clock::time_point ts) {
        std::lock_guard lock(mutex_);
        manual_ts_ = ts;
    }

private:
    std::optional<std::chrono::system_clock::time_point> manual_ts_;
    std::mutex mutex_;
};
