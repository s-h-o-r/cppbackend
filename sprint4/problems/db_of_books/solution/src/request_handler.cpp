#include <iostream>

#include "request_handler.h"

namespace handler {

using namespace std::literals;

void AddBookQuery::Process(const book_manager::BookInfo& book_info) {
    bool res = book_manager_->AddBook(book_info);
    std::cout << json_loader::PrepareResultAddBook(res) << std::endl;
}

void PrintBooksQuery::Process() const {
    auto all_books = book_manager_->GetAllBooks();
    std::cout << json_loader::PrepareResultAllBooks(all_books) << std::endl;
}

void RequestHandler::ProcessQuery() {
    std::string raw_query;
    if (std::getline(std::cin, raw_query)) {
        OnQuery(json_loader::ParseQuery(std::move(raw_query))); // TO DO: make concurrency
    }
}

void RequestHandler::OnQuery(json_loader::Query query) {
    if (query.name == "add_book"s) {
        if (!query.payload) {
            throw std::logic_error("Payload should not be empty for add_book command"s);
        }
        add_book_query_.Process(*query.payload);
    } else if (query.name == "all_books"s) {
        print_books_query_.Process();
    }

    if (query.name != "exit"s) {
        ProcessQuery();
    }
}

} // namespace handler
