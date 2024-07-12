#ifdef WIN32
#include <sdkddkver.h>
#endif
// boost.beast будет использовать std::string_view вместо boost::string_view
#define BOOST_BEAST_USE_STD_STRING_VIEW

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <iostream>
#include <thread>
#include <optional>

namespace net = boost::asio;
using tcp = net::ip::tcp;
using namespace std::literals;
namespace beast = boost::beast;
namespace http = beast::http;

using StringRequest = http::request<http::string_body>;
using StringResponse = http::response<http::string_body>;

std::optional<StringRequest> ReadRequest(tcp::socket& socket, beast::flat_buffer& buffer) {
    beast::error_code ec;
    StringRequest req;

    http::read(socket, buffer, req, ec);

    if (ec == http::error::end_of_stream) {
        return std::nullopt;
    }

    if (ec) {
        throw std::runtime_error("Faild to read request: "s.append(ec.message()));
    }
    return req;
}

class RequestHandler {
public:
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

template <typename RequestHandle>
void HandleConnection(tcp::socket& socket, RequestHandle&& handle_request) {
    try {
        beast::flat_buffer buffer;
        while (auto request = ReadRequest(socket, buffer)) {
            StringResponse response = handle_request(std::move(*request));
            http::write(socket, response);
            if (response.need_eof()) {
                break;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
}

int main() {
    net::io_context ioc;

    const auto address = net::ip::make_address("0.0.0.0");
    constexpr unsigned short port = 8080;

    tcp::acceptor acceptor(ioc, {address, port});
    std::cout << "Server has started..."sv << std::endl;

    while (true) {
        tcp::socket socket(ioc);
        acceptor.accept(socket);

        auto connection_hadler = [](tcp::socket socket) {
            HandleConnection(socket, RequestHandler());
        };

        std::thread t(connection_hadler, std::move(socket));
        t.detach();
    }
}
