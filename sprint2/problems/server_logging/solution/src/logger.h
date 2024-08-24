#include <boost/asio/ip/tcp.hpp>
#include <boost/log/trivial.hpp>

#include <chrono>
#include <string_view>

using namespace std::literals;
namespace net = boost::asio;

namespace http_logger {

template <typename Request>
static void LogRequest(std::string_view client_ip, Request& request);

template <typename Response>
static void LogResponse();


static void LogServerStart(std::string_view port, std::string_view adress);
static void LogServerEnd(int code, std::string_view message);
static void LogServerError(int code, std::string_view message, std::string_view where);
} // namespace logger
