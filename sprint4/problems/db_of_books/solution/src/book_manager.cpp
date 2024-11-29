#include "book_manager.h"

#include <string>
#include <sstream>

#include <pqxx/zview>

using namespace std::literals;
using pqxx::operator""_zv;

namespace book_manager {

BookManager::BookManager(const char* connection)
: conn_(connection) {
    pqxx::work work{conn_};

    std::ostringstream query;
    query << "CREATE TABLE IF NOT EXISTS books ("
            << "id SERIAL PRIMARY KEY" << ", "
            << "title varchar(100) NOT NULL" << ", "
            << "author varchar(100) NOT NULL" << ", "
            << "year integer NOT NULL CHECK (year > 0)" << ", "
            << "isbn char(13) UNIQUE"
    << ");";

    work.exec(query.view());
    work.commit();
}

bool BookManager::AddBook(const BookInfo& book_info) {
    if (!(book_info.title && book_info.author)) {
        return false;
    }

    try {
        pqxx::work work{conn_};

        std::ostringstream query;
        query << "INSERT INTO books VALUES (DEFAULT, '" << work.esc(*book_info.title)
        << "', '" << work.esc(*book_info.author) << "', " << std::to_string(book_info.year)
        << ", " << (!book_info.isbn ? "NULL" : "'" + work.esc(*book_info.isbn) + "'")
        << ");";

        work.exec(query.view());
        work.commit();
    } catch (...) {
        return false;
    }

    return true;
}

std::vector<BookInfo> BookManager::GetAllBooks() const {
    std::vector<BookInfo> books;

    pqxx::read_transaction read{const_cast<pqxx::connection&>(conn_)}; // I want to save method const, but I can't do this without const_cast
    auto query_text = "SELECT * FROM books ORDER BY year DESC, title, author, isbn;"_zv;
    for (auto [id, title, author, year, isbn] :
         read.query<std::uint32_t, std::string, std::string, int, std::optional<std::string>>(query_text)) {
        books.push_back({id, title, author, year, isbn});
    }

    return books;
}

void BookManager::ClearDatabase() {
    pqxx::work work{conn_};
    work.exec("DELETE FROM books;"sv);
    work.commit();
}

} // namespace book_manager
