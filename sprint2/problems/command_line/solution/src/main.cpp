#include "sdk.h"

#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <iostream>
#include <memory>
#include <thread>

#include "app.h"
#include "cl_parser.h"
#include "json_loader.h"
#include "logger.h"
#include "request_handler.h"
#include "ticker.h"

using namespace std::literals;
namespace net = boost::asio;

namespace {

// Запускает функцию fn на n потоках, включая текущий
template <typename Fn>
void RunWorkers(unsigned n, const Fn& fn) {
    n = std::max(1u, n);
    #ifndef __clang__
    std::vector<std::jthread> workers;
    #else
    std::vector<std::thread> workers;
    #endif
    workers.reserve(n - 1);
    // Запускаем n-1 рабочих потоков, выполняющих функцию fn
    while (--n) {
        workers.emplace_back(fn);
    }
    fn();
    #ifdef __clang__
    for (auto& worker : workers) {
        worker.join();
    }
    #endif
}

}  // namespace

int main(int argc, const char* argv[]) {
    cl_parser::Args cl_args;
    try {
        if (auto args = cl_parser::ParseComandLine(argc, argv)) {
            cl_args = *args;
        } else {
            return 0;
        }
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << std::endl;
        return EXIT_FAILURE;
    }

    try {
        // 1. Загружаем карту из файла и построить модель игры
        model::Game game = json_loader::LoadGame(cl_args.config_file_path);
        if (cl_args.random_spawn_point) {
            game.TurnOnRandomSpawn();
        }

        // 2. Инициализируем io_context
        const unsigned num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(num_threads);

        // 3. Добавляем асинхронный обработчик сигналов SIGINT и SIGTERM
        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const boost::system::error_code& ec, [[maybe_unused]] int signal_number) {
            if (!ec) {
                ioc.stop();
            }
        });

        // 4. Создаём обработчик HTTP-запросов и связываем его с моделью игры
        app::Application app(&game);
        auto game_state_strand = net::make_strand(ioc);

        auto handler = std::make_shared<http_handler::RequestHandler>(app, ioc, game_state_strand,
                                                                      std::move(cl_args.static_root), !(static_cast<bool>(cl_args.tick_period)));
        http_logger::InitBoostLogFilter(http_logger::LogFormatter);
        http_logger::LogginRequestHandler<http_handler::RequestHandler> logging_handler(*handler);

        if (cl_args.tick_period != 0) {
            auto tick_period = std::chrono::milliseconds{cl_args.tick_period};
            auto ticker = std::make_shared<tick::Ticker>(game_state_strand, tick_period, [&app](std::chrono::milliseconds delta) {
                app.ProcessTick(delta.count());
            });
            ticker->Start();
        }

        // 5. Запустить обработчик HTTP-запросов, делегируя их обработчику запросов
        const auto address = net::ip::make_address("0.0.0.0");
        constexpr net::ip::port_type port = 8080;
        http_server::ServeHttp(ioc, {address, port}, [&logging_handler](auto&& req, auto client_ip, auto&& send) {
            logging_handler(std::forward<decltype(req)>(req), client_ip, std::forward<decltype(send)>(send));
        });

        // Эта надпись сообщает тестам о том, что сервер запущен и готов обрабатывать запросы
        http_logger::LogServerStart(port, address.to_string());

        // 6. Запускаем обработку асинхронных операций
        RunWorkers(std::max(1u, num_threads), [&ioc] {
            ioc.run();
        });

        http_logger::LogServerEnd(0, ""sv);
    } catch (const std::exception& ex) {
        http_logger::LogServerEnd(EXIT_FAILURE, ex.what());
        std::cerr << ex.what() << std::endl;
        return EXIT_FAILURE;
    }
}
