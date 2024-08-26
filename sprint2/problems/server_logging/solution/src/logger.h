#pragma once

#include <boost/beast/http/verb.hpp>
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
BOOST_LOG_ATTRIBUTE_KEYWORD(message, "Message", std::string_view)
BOOST_LOG_ATTRIBUTE_KEYWORD(code, "Code", unsigned int)
BOOST_LOG_ATTRIBUTE_KEYWORD(ip, "IP", std::string_view)
BOOST_LOG_ATTRIBUTE_KEYWORD(uri, "URI", std::string_view)
BOOST_LOG_ATTRIBUTE_KEYWORD(method, "Method", std::string_view)
BOOST_LOG_ATTRIBUTE_KEYWORD(response_time, "Response_time", long long)
BOOST_LOG_ATTRIBUTE_KEYWORD(content_type, "ContentType", std::string_view)
BOOST_LOG_ATTRIBUTE_KEYWORD(server_port, "ServerPort", unsigned int)
BOOST_LOG_ATTRIBUTE_KEYWORD(client_address, "ClientAddress", std::string_view)
BOOST_LOG_ATTRIBUTE_KEYWORD(exception, "Exception", std::string_view)
BOOST_LOG_ATTRIBUTE_KEYWORD(error_text, "Text", std::string_view)
BOOST_LOG_ATTRIBUTE_KEYWORD(error_where, "Where", std::string_view)

void LogRequestFormatter(logging::record_view const& rec, logging::formatting_ostream& strm);
void LogResponseFormatter(logging::record_view const& rec, logging::formatting_ostream& strm);
void LogServerStartFormatter(logging::record_view const& rec, logging::formatting_ostream& strm);
void LogServerEndFormatter(logging::record_view const& rec, logging::formatting_ostream& strm);
void LogServerErrorFormatter(logging::record_view const& rec, logging::formatting_ostream& strm);

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
    InitBoostLogFilter(LogRequestFormatter);
    BOOST_LOG_TRIVIAL(info) << logging::add_value(ip, client_ip)
                            << logging::add_value(uri, request.target())
                            << logging::add_value(method, http::to_string(request.method()))
                            << "request received";
}

template <typename Response>
void LogResponse(long long time, Response& response) {
    InitBoostLogFilter(LogResponseFormatter);
    BOOST_LOG_TRIVIAL(info) << logging::add_value(response_time, time)
                            << logging::add_value(code, response.result_int())
                            << logging::add_value(content_type, response["content_type"])
                            << "response sent";
}


void LogServerStart(unsigned int port, std::string_view address);
void LogServerEnd(unsigned int code, std::string_view exeption_text);
void LogServerError(unsigned int code, std::string_view message, std::string_view where);
} // namespace http_logger
