#pragma once
#ifdef _WIN32
#include <sdkddkver.h>
#endif

#include <atomic>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <memory>

#include "hotdog.h"
#include "result.h"

namespace net = boost::asio;
namespace sys = boost::system;
using Timer = net::steady_timer;

class ThreadChecker {
public:
    explicit ThreadChecker(std::atomic_int& counter)
        : counter_{counter} {
    }

    ThreadChecker(const ThreadChecker&) = delete;
    ThreadChecker& operator=(const ThreadChecker&) = delete;

    ~ThreadChecker() {
        // assert выстрелит, если между вызовом конструктора и деструктора
        // значение expected_counter_ изменится
        assert(expected_counter_ == counter_);
    }

private:
    std::atomic_int& counter_;
    int expected_counter_ = ++counter_;
};

// Функция-обработчик операции приготовления хот-дога
using HotDogHandler = std::function<void(Result<HotDog> hot_dog)>;

class Order : public std::enable_shared_from_this<Order> {
public:
    Order(net::io_context& ioc, int order_id, HotDogHandler handler, GasCooker& gas_cooker,
          std::shared_ptr<Sausage> sausage, std::shared_ptr<Bread> bread)
        : ioc_(ioc)
        , order_id_(order_id)
        , handler_(std::move(handler))
        , gas_cooker_(gas_cooker)
        , sausage_(sausage)
        , bread_(bread) {
    }
    
    void Execute() {
        BakeBread();
        FrySausage();
    }

private:
    net::io_context& ioc_;
    int order_id_;
    HotDogHandler handler_;
    GasCooker& gas_cooker_;
    std::shared_ptr<Sausage> sausage_;
    std::shared_ptr<Bread> bread_;


    std::atomic_int counter_ = 0;
    net::strand<net::io_context::executor_type> strand_{net::make_strand(ioc_)};
    Timer frying_timer_{strand_};
    Timer baking_timer_{strand_};

    bool delivered_ = false;

    void BakeBread() {
        bread_->StartBake(gas_cooker_, [self = shared_from_this()]() {
            self->baking_timer_.expires_after(HotDog::MIN_BREAD_COOK_DURATION);
            self->baking_timer_.async_wait(
                net::bind_executor(self->strand_, [self](sys::error_code ec) {
                self->OnBaked(ec);
            }));
        });
    }

    void OnBaked(sys::error_code ec) {
        ThreadChecker thread_checker(counter_);
        bread_->StopBaking();
        CheckReadiness(ec);
    }

    void FrySausage() {
        sausage_->StartFry(gas_cooker_, [self = shared_from_this()]() {
            self->frying_timer_.expires_after(HotDog::MIN_SAUSAGE_COOK_DURATION);
            self->frying_timer_.async_wait(
                net::bind_executor(self->strand_, [self](sys::error_code ec) {
                self->OnFried(ec);
            }));
        });
    }

    void OnFried(sys::error_code ec) {
        ThreadChecker thread_checker(counter_);
        sausage_->StopFry();
        CheckReadiness(ec);
    }

    void CheckReadiness(sys::error_code ec) {
        if (delivered_) {
            return;
        }

        if (ec) {
            Deliver(ec);
            return;
        }

        if (IsReadyToDeliver()) {
            Deliver({});
        }
    }

    void Deliver(sys::error_code ec) {
        if (ec) {
            handler_(Result<HotDog>{std::make_exception_ptr(std::runtime_error{ec.what()})});
        } else {
            handler_(Result<HotDog>{{order_id_, sausage_, bread_}});
        }
    }

    bool IsReadyToDeliver() const {
        return sausage_->IsCooked() && bread_->IsCooked();
    }
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
        const int order_id = ++next_order_id_;
        std::shared_ptr<Sausage> sausage;
        std::shared_ptr<Bread> bread;
        {
            std::lock_guard lk{mutex};
            ThreadChecker thread_checker(counter_);
            sausage = store_.GetSausage();
            bread = store_.GetBread();
        }
        std::make_shared<Order>(io_, order_id, std::move(handler),
                                *gas_cooker_, sausage, bread)->Execute();
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
    int next_order_id_ = 0;
    std::atomic_int counter_ = 0;
    std::mutex mutex;

    net::strand<net::io_context::executor_type> strand_{net::make_strand(io_)};
};
