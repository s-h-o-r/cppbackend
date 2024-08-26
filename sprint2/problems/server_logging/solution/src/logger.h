#pragma once

#include <boost/beast/http/verb.hpp>
#include <boost/json.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>

#include <chrono>
#include <iostream>
#include <string_view>

namespace logging = boost::log;
namespace keywords = boost::log::keywords;
namespace http = boost::beast::http;

namespace http_logger {

class DurationMeasure {
public:
    DurationMeasure() = default;

    auto Count() {
        std::chrono::system_clock::time_point end_ts = std::chrono::system_clock::now();
        return (end_ts - start_ts_).count();
    };

private:
    std::chrono::system_clock::time_point start_ts_ = std::chrono::system_clock::now();
};

BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime)
BOOST_LOG_ATTRIBUTE_KEYWORD(log_data, "LogData", boost::json::value)

void LogFormatter(logging::record_view const& rec, logging::formatting_ostream& strm);

template <typename Formatter>
void InitBoostLogFilter(Formatter&& formatter) {
    logging::add_common_attributes();
    
    logging::add_console_log(
        std::cout,
        keywords::format = &formatter,
        keywords::auto_flush = true
    );
}

template <typename Request>
void LogRequest(std::string_view client_ip, Request& request) {
    InitBoostLogFilter(LogFormatter);
    boost::json::value data = {
        {"ip", client_ip},
        {"URI", request.target()},
        {"method", http::to_string(request.method())}
    };
    BOOST_LOG_TRIVIAL(info) << logging::add_value(log_data, data) << "request received";
}

template <typename Response>
void LogResponse(long long time, Response& response) {
    InitBoostLogFilter(LogFormatter);
    auto content_type = response["content_type"];
    if (content_type.empty()) {
        content_type = "null";
    }
    boost::json::value data = {
        {"response_time", time},
        {"code", response.result_int()},
        {"content_type", content_type}
    };
    BOOST_LOG_TRIVIAL(info) << logging::add_value(log_data, data) << "response sent";
}


void LogServerStart(unsigned int port, std::string_view address);
void LogServerEnd(unsigned int code, std::string_view exeption_text);
void LogServerError(unsigned int code, std::string_view message, std::string_view where);
} // namespace http_logger
