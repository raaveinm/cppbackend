//
// Created by raaveinm on 6/25/26.
//
#include <ranges>
#ifdef WIN32
#include <sdkddkver.h>
#endif

#include <thread>
#include <chrono>
#include <functional>
#include <iostream>
#include <sstream>
#include <string_view>
#include <syncstream>
#include <boost/asio.hpp>
#include <atomic>

namespace net = boost::asio;
namespace sys = boost::system;

class ThreadChecker {
public:
    explicit ThreadChecker(std::atomic_int& counter)
        : counter_{counter} {
    }

    ThreadChecker(const ThreadChecker&) = delete;
    ThreadChecker& operator=(const ThreadChecker&) = delete;

    ~ThreadChecker() {
        assert(expected_counter_ == counter_);
    }

private:
    std::atomic_int& counter_;
    int expected_counter_ = ++counter_;
};

class Hamburger {
public:
    [[nodiscard]] bool IsCutletRoasted() const { return cutlet_roasted_; }
    [[nodiscard]] bool HasOnion() const { return has_onion_; }
    [[nodiscard]] bool IsPacked() const { return is_packed_; }

    void SetCutletRoasted() {
        if (IsCutletRoasted())
            throw std::logic_error("Cutlet has been roasted already");

        cutlet_roasted_ = true;
    }

    void AddOnion() {
        if (IsPacked())
            throw std::logic_error("Hamburger has been packed already");

        AssureCutletRoasted();
        has_onion_ = true;
    }

    void Pack() {
        AssureCutletRoasted();
        is_packed_ = true;
    }

private:
    void AssureCutletRoasted() const {
        if (!cutlet_roasted_) {
            throw std::logic_error("Bread has not been roasted yet");
        }
    }

    bool cutlet_roasted_ = false;
    bool has_onion_ = false;
    bool is_packed_ = false;
};

std::ostream& operator<<(std::ostream& os, const Hamburger& h) {
    return os << "Hamburger: " << (h.IsCutletRoasted() ? "roasted cutlet" : " raw cutlet")
              << (h.HasOnion() ? ", onion" : "")
              << (h.IsPacked() ? ", packed" : ", not packed");
}

class Logger {
public:
    explicit Logger(std::string id)
        : id_(std::move(id)) {
    }

    void LogMessage(const std::string_view message) const {
        std::osyncstream os{std::cout};
        os << id_ << "> [" << std::chrono::duration<double>(std::chrono::steady_clock::now() - start_time_).count()
           << "s] " << message << std::endl;
    }

private:
    std::string id_;
    std::chrono::steady_clock::time_point start_time_{std::chrono::steady_clock::now()};
};

using OrderHandler = std::function<void(sys::error_code err, int id, Hamburger* hamburger)>;

class Order : public std::enable_shared_from_this<Order> {
public:
    Order(net::io_context& io, const int id, const bool with_onion, OrderHandler handler)
        : io_(io),
            id_(id),
            with_onion_(with_onion),
            handler_(std::move(handler)) {
    }

    void Execute() {
        logger_.LogMessage("Order has ben started");
        RoastCutlet();
        if (with_onion_) MarinadeOnion();
    }

private:
    void RoastCutlet() {
        logger_.LogMessage("Start roasting cutlet");
        roast_timer_.async_wait(
            net::bind_executor(strand_, [self = shared_from_this()](const sys::error_code &ec) {
                self->OnRoasted(ec);
            }));
    }

    void OnRoasted(const sys::error_code &ec) {
        if (ec) {
            logger_.LogMessage("Roast error : " + ec.what());
        } else {
            logger_.LogMessage("Cutlet has been roasted.");
            hamburger_.SetCutletRoasted();
        }
        CheckReadiness(ec);
    }

    void MarinadeOnion() {
        logger_.LogMessage("Start marinading onion");
        marinade_timer_.async_wait(
            net::bind_executor(strand_, [self = shared_from_this()](const sys::error_code &ec) {
                self->OnOnionMarinaded(ec);
            }));
    }

    void OnOnionMarinaded(sys::error_code ec) {
        if (ec) {
            logger_.LogMessage("Marinade onion error: " + ec.what());
        } else {
            logger_.LogMessage("Onion has been marinaded.");
            onion_marinaded_ = true;
        }
        CheckReadiness(ec);
    }

    void CheckReadiness(const sys::error_code &ec) {
        if (delivered_) return;
        if (ec) return Deliver(ec);

        if (CanAddOnion()) {
            logger_.LogMessage("Add onion");
            hamburger_.AddOnion();
        }
        if (IsReadyToPack()) {
            Pack();
        }
    }

    void Deliver(sys::error_code ec) {
        delivered_ = true;
        handler_(ec, id_, ec ? nullptr : &hamburger_);
    }

    [[nodiscard]] bool CanAddOnion() const {
        return hamburger_.IsCutletRoasted() && onion_marinaded_ && !hamburger_.HasOnion();
    }

    [[nodiscard]] bool IsReadyToPack() const {
        return hamburger_.IsCutletRoasted() && (!with_onion_ || hamburger_.HasOnion());
    }

    void Pack() {
        logger_.LogMessage("Packing");

        const auto start = std::chrono::steady_clock::now();
        while (std::chrono::steady_clock::now() - start < std::chrono::milliseconds(500)) {}

        hamburger_.Pack();
        logger_.LogMessage("Packed");
        Deliver({});
    }

    net::io_context& io_;
    int id_;
    bool with_onion_;
    OrderHandler handler_;
    Logger logger_{std::to_string(id_)};

    Hamburger hamburger_;
    bool onion_marinaded_ = false;
    bool delivered_ = false;

    net::steady_timer roast_timer_{io_, std::chrono::seconds(1)};
    net::steady_timer marinade_timer_{io_, std::chrono::seconds(2)};
    net::strand<net::io_context::executor_type> strand_{net::make_strand(io_)};
};

class Restaurant {
public:
    explicit Restaurant(net::io_context& io)
        : io_(io) {
    }

    int MakeHamburger(bool with_onion, OrderHandler handler) {
        const int order_id = ++next_order_id_;
        std::make_shared<Order>(io_, order_id, with_onion, std::move(handler))->Execute();
        return order_id;
    }

private:
    net::io_context& io_;
    int next_order_id_ = 0;
};

int main() {
    net::io_context io;
    Restaurant restaurant{io};

    restaurant.MakeHamburger(true, [](sys::error_code ec, int order_id, Hamburger* h) {});

    io.run();
}