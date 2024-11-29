#pragma once

#include <string>

#include "book_manager.h"
#include "json_loader.h"

namespace handler {

class AddBookQuery {
public:
    AddBookQuery(book_manager::BookManager* book_manager)
    : book_manager_(book_manager) {
    }

    void Process(const book_manager::BookInfo& book_info);

private:
    book_manager::BookManager* book_manager_;
};

class PrintBooksQuery {
public:
    PrintBooksQuery(const book_manager::BookManager* book_manager)
    : book_manager_(book_manager) {
    }

    void Process() const;

private:
    const book_manager::BookManager* book_manager_;
};


class RequestHandler {
public:

    RequestHandler(book_manager::BookManager* book_manager)
    : book_manager_(book_manager) {
    }

    void ProcessQuery();

private:
    book_manager::BookManager* book_manager_;

    // request scenarios
    AddBookQuery add_book_query_{book_manager_};
    PrintBooksQuery print_books_query_{book_manager_};

    void OnQuery(json_loader::Query query);
};
} // namespace handler
