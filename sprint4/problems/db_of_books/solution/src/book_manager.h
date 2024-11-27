#pragma once

#include <pqxx/pqxx>

#include <optional>
#include <string>
#include <string_view>

namespace book_manager {

struct BookInfo {
    int id;
    std::string title;
    std::string author;
    int year;
    std::optional<std::string> isbn;
};

class BookManager {
public:
    explicit BookManager(const char* connection)
    : conn_(connection)
    , work_(conn_)
    , read_trans_(conn_) {
    }

    void AddBook();


private:
    pqxx::connection conn_;
    pqxx::work work_;
    pqxx::read_transaction read_trans_;
};

} // namespace book_manager
