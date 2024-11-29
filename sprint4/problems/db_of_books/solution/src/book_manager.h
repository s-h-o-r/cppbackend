#pragma once

#include <pqxx/pqxx>

#include <optional>
#include <string>
#include <string_view>

namespace book_manager {

struct BookInfo {
    std::uint32_t id = -1;
    std::optional<std::string> title;
    std::optional<std::string> author;
    int year;
    std::optional<std::string> isbn;
};

class BookManager {
public:
    explicit BookManager(const char* connection);

    bool AddBook(const BookInfo& book_info);
    std::vector<BookInfo> GetAllBooks() const;
    void ClearDatabase();

private:
    pqxx::connection conn_;
};

} // namespace book_manager
