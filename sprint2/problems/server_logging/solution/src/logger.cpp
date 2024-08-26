#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/date_time.hpp>

#include <chrono>
#include <string_view>

#include "logger.h"

using namespace std::literals;

namespace logging = boost::log;
namespace keywords = boost::log::keywords;
namespace expr = boost::log::expressions;

namespace http_logger {

void LogRequestFormatter(logging::record_view const& rec, logging::formatting_ostream& strm) {
    auto ts = rec[timestamp];
    strm << "{\"timestamp\":\""
        << to_iso_extended_string(*ts) << "\","
        << "\"data\":{\"ip\":\"" << rec[ip] << "\","
        << "\"URI\":\"" << rec[uri] << "\","
        << "\"method\":\"" << rec[method] << "\"},"
        << "\"message\":\"" << rec[expr::smessage] << "\"}";
}

void LogResponseFormatter(logging::record_view const& rec, logging::formatting_ostream& strm) {
    auto ts = rec[timestamp];
    strm << "{\"timestamp\":\""
        << to_iso_extended_string(*ts) << "\","
        << "\"data\":{\"ip\":\"" << rec[ip] << "\","
        << "\"response_time\":" << rec[response_time] << ","
        << "\"code\":" << rec[code] << ","
        << "\"content_type\":\"" << rec[content_type] << "\"},"
        << "\"message\":\"" << rec[expr::smessage] << "\"}";
}

void LogServerStartFormatter(logging::record_view const& rec, logging::formatting_ostream& strm) {
    auto ts = rec[timestamp];
    strm << "{\"timestamp\":\""
        << to_iso_extended_string(*ts) << "\","
    << "\"data\":{\"port\":" << rec[server_port] << ","
        << "\"address\":\"" << rec[client_address] << "\"},"
        << "\"message\":\"" << rec[expr::smessage] << "\"}";
}

void LogServerEndFormatter(logging::record_view const& rec, logging::formatting_ostream& strm) {
    auto ts = rec[timestamp];
    strm << "{\"timestamp\":\""
        << to_iso_extended_string(*ts) << "\","
        << "\"data\":{\"code\":" << rec[code] << ",";

    if (rec[code] != 0) {
        strm << "\"exception\":\"" << rec[exception] << "\"";
    }

    strm << "},\"message\":\"" << rec[expr::smessage] << "\"}";
}

void LogServerErrorFormatter(logging::record_view const& rec, logging::formatting_ostream& strm) {
    auto ts = rec[timestamp];
    strm << "{\"timestamp\":\""
        << to_iso_extended_string(*ts) << "\","
        << "\"data\":{\"code\":" << rec[code] << ","
        << "\"text\":\"" << rec[message] << "\",";

    if (rec[code] != 0) {
        strm << "\"where\":\"" << rec[error_where] << "\"";
    }

    strm << "},\"message\":\"" << rec[expr::smessage] << "\"}";
}

void LogServerStart(unsigned int port, std::string_view address) {
    InitBoostLogFilter(LogServerStartFormatter);
    BOOST_LOG_TRIVIAL(info) << logging::add_value(server_port, port)
                            << logging::add_value(client_address, address)
                            << "server started";
}

void LogServerEnd(unsigned int return_code, std::string_view exeption_text) {
    InitBoostLogFilter(LogServerEndFormatter);
    BOOST_LOG_TRIVIAL(info) << logging::add_value(code, return_code)
                            << logging::add_value(exception, exeption_text)
                            << "server exited";
}

void LogServerError(unsigned int error_code, std::string_view error_message, std::string_view where) {
    InitBoostLogFilter(LogServerErrorFormatter);
    BOOST_LOG_TRIVIAL(info) << logging::add_value(code, error_code)
                            << logging::add_value(error_text, error_message)
                            << logging::add_value(error_where, where)
                            << "error";
}

} // namespace http_logger
