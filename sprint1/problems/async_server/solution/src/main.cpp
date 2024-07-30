#include "sdk.h"
//
#include <boost/asio/signal_set.hpp>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

#include "http_server.h"

namespace {
namespace net = boost::asio;
using namespace std::literals;
namespace sys = boost::system;
namespace http = boost::beast::http;

// Запрос, тело которого представлено в виде строки
using StringRequest = http::request<http::string_body>;
// Ответ, тело которого представлено в виде строки
using StringResponse = http::response<http::string_body>;

class RequestHandler {
public:
    StringResponse operator()(StringRequest&& req) {
        StringResponse response;
        response.version(req.version());
        response.keep_alive(req.keep_alive());

        std::string target = req.target();
        target.erase(target.begin());

        std::string body = "Hello"s;
        if (!target.empty()) body.append(", "s + target);

        if (req.method() != http::verb::get && req.method() != http::verb::head) {
            response.body() = "Invalid method"sv;
            MakeBasicResponseInfo(response, http::status::method_not_allowed, response.body().size());
            response.set(http::field::allow, "GET, HEAD"sv);
            return response;
        }


        if (req.method() == http::verb::get) {
            response.body() = body;
        }

        MakeBasicResponseInfo(response, http::status::ok, body.size());
        return response;
    }

private:
    struct ContentType {
        ContentType() = delete;
        constexpr static std::string_view TEXT_HTML = "text/html"sv;
    };

    void MakeBasicResponseInfo(StringResponse& response, http::status status, size_t body_size,
                               std::string_view content_type = ContentType::TEXT_HTML) {
        response.result(status);
        response.set(http::field::content_type, content_type);
        response.content_length(body_size);
    }
};
// Запускает функцию fn на n потоках, включая текущий
template <typename Fn>
void RunWorkers(unsigned n, const Fn& fn) {
    n = std::max(1u, n);
    std::vector<std::jthread> workers;
    workers.reserve(n - 1);
    // Запускаем n-1 рабочих потоков, выполняющих функцию fn
    while (--n) {
        workers.emplace_back(fn);
    }
    fn();
}

}  // namespace

int main() {
    const unsigned num_threads = std::thread::hardware_concurrency();

    net::io_context ioc(num_threads);

    // Подписываемся на сигналы и при их получении завершаем работу сервера
    net::signal_set signals(ioc, SIGINT, SIGTERM);
    signals.async_wait([&ioc](const sys::error_code& ec, [[maybe_unused]] int signal_number) {
        if (!ec) {
            ioc.stop();
        }
    });

    const auto address = net::ip::make_address("0.0.0.0");
    constexpr net::ip::port_type port = 8080;
    http_server::ServeHttp(ioc, {address, port}, [](auto&& req, auto&& sender) {
        sender(RequestHandler()(std::forward<decltype(req)>(req)));
    });

    // Эта надпись сообщает тестам о том, что сервер запущен и готов обрабатывать запросы
    std::cout << "Server has started..."sv << std::endl;

    RunWorkers(num_threads, [&ioc] {
        ioc.run();
    });
}
