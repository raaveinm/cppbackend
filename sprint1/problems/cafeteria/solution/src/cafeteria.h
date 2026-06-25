#pragma once
#ifdef _WIN32
#include <sdkddkver.h>
#endif

#include <atomic>
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <memory>

#include "hotdog.h"
#include "result.h"

namespace net = boost::asio;

// Функция-обработчик операции приготовления хот-дога
using HotDogHandler = std::function<void(Result<HotDog> hot_dog)>;


class OrderSession : public std::enable_shared_from_this<OrderSession> {
public:
    OrderSession(net::io_context& io, Store& store, std::shared_ptr<GasCooker> cooker, HotDogHandler handler)
        : strand_{net::make_strand(io)}
        , timer_bread_{io}
        , timer_sausage_{io}
        , store_{store}
        , cooker_{std::move(cooker)}
        , handler_{std::move(handler)} {
        static std::atomic<int> next_delivery_id{1};
        id_ = next_delivery_id++;
    }

    void Start() {
        bread_ = store_.GetBread();
        sausage_ = store_.GetSausage();
        StartBreadWorkflow();
        StartSausageWorkflow();
    }

private:
    void StartBreadWorkflow() {
        bread_->StartBake(*cooker_, [self = shared_from_this()]() {
            self->timer_bread_.expires_after(HotDog::MIN_BREAD_COOK_DURATION);

            self->timer_bread_.async_wait([self](const boost::system::error_code& ec) {
                if (!ec) self->bread_->StopBaking();

                net::dispatch(self->strand_, [self]() {
                    self->OnIngredientReady();
                });
            });
        });
    }

    void StartSausageWorkflow() {
        sausage_->StartFry(*cooker_, [self = shared_from_this()]() {
            self->timer_sausage_.expires_after(HotDog::MIN_SAUSAGE_COOK_DURATION);

            self->timer_sausage_.async_wait([self](const boost::system::error_code& ec) {
                if (!ec) {
                    self->sausage_->StopFry();
                }
                net::dispatch(self->strand_, [self]() {
                    self->OnIngredientReady();
                });
            });
        });
    }

    void OnIngredientReady() {
        ++ready_ingredients_count_;
        if (ready_ingredients_count_ == 2)
            ConstructHotDog();
    }

    void ConstructHotDog() const {
        try {
            HotDog hot_dog(id_, sausage_, bread_);
            handler_(Result<HotDog>{std::move(hot_dog)});
        } catch (...) {
            handler_(Result<HotDog>::FromCurrentException());
        }
    }

    net::strand<net::io_context::executor_type> strand_;

    net::steady_timer timer_bread_;
    net::steady_timer timer_sausage_;

    Store& store_;
    std::shared_ptr<GasCooker> cooker_;
    HotDogHandler handler_;

    std::shared_ptr<Bread> bread_;
    std::shared_ptr<Sausage> sausage_;

    int id_;
    int ready_ingredients_count_ = 0;
};

// Класс "Кафетерий". Готовит хот-доги
class Cafeteria {
public:
    explicit Cafeteria(net::io_context& io)
        : io_{io} {
    }

    // Асинхронно готовит хот-дог и вызывает handler, как только хот-дог будет готов.
    // Этот метод может быть вызван из произвольного потока
    void OrderHotDog(HotDogHandler handler) {
        const auto session = std::make_shared<OrderSession>(io_, store_, gas_cooker_, std::move(handler));
        session->Start();
    }

private:
    net::io_context& io_;
    // Используется для создания ингредиентов хот-дога
    Store store_;
    // Газовая плита. По условию задачи в кафетерии есть только одна газовая плита на 8 горелок
    // Используйте её для приготовления ингредиентов хот-дога.
    // Плита создаётся с помощью make_shared, так как GasCooker унаследован от
    // enable_shared_from_this.
    std::shared_ptr<GasCooker> gas_cooker_ = std::make_shared<GasCooker>(io_);
};