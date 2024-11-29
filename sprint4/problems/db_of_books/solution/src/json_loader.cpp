#include "json_loader.h"

#define BOOST_BEAST_USE_STD_STRING_VIEW

namespace json_loader {

using namespace std::literals;
namespace sys = boost::system;
namespace json = boost::json;

Query ParseQuery(std::string query) {
    sys::error_code ec;
    json::value game_info{json::parse(query, ec)};

    if (ec) {
        throw std::logic_error("cannot parse query");
    }

    Query query_info;
    query_info.name = game_info.at("action"sv).as_string();

    if (query_info.name == "add_book"s) {
        json::object payload = game_info.at("payload"sv).as_object();
        query_info.payload = book_manager::BookInfo{
            .title{payload.at("title"sv).as_string()},
            .author{payload.at("author"sv).as_string()},
            .year = static_cast<int>(payload.at("year"sv).as_int64())};

        if (payload.at("ISBN").kind() != json::kind::null) {
            query_info.payload->isbn = payload.at("ISBN").as_string();
        }
    }

    return query_info;
}

std::string PrepareResultAddBook(bool action_result) {
    json::object res;
    res.emplace("result"sv, action_result);

    return json::serialize(res);
}

std::string PrepareResultAllBooks(const std::vector<book_manager::BookInfo>& books) {
    json::array json_books;
    for (const auto& book : books) {
        json_books.emplace_back(json::value(json::object{
            {"id"sv, book.id},
            {"title"sv, *book.title},
            {"author"sv, *book.author},
            {"year"sv, book.year},
            {"ISBN"sv, !book.isbn ? json::value() : json::value(*book.isbn)}
        }));
    }
    return json::serialize(json_books);
}

} // namespace json_loader
