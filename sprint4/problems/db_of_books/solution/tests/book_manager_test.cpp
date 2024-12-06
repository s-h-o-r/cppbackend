#include <catch2/catch_test_macros.hpp>

#include "../src/book_manager.h"

using namespace book_manager;
using namespace std::literals;
using pqxx::operator"" _zv;

struct Fixture {
    BookManager book_manager{"postgres://test:test@localhost:5432/book_db_test"};
};

TEST_CASE_METHOD(Fixture, "BookManager unit test") {
    book_manager.ClearDatabase();
    REQUIRE(book_manager.GetAllBooks().empty());

    SECTION("call AddBook method") {
        // emptry string is NULL
        BookInfo book{.author = "Author"s, .year = 2024, .isbn = std::nullopt}; // .title is NULL
        CHECK(!book_manager.AddBook(book));

        book.title = "101 symbols 11111111111111111111111111111111111111111111111111111111111111111111111111111111111111111"s;
        CHECK(!book_manager.AddBook(book));

        book.title = "Book"s;
        book.author = std::nullopt;
        CHECK(!book_manager.AddBook(book));

        book.author = "101 symbols 11111111111111111111111111111111111111111111111111111111111111111111111111111111111111111"s;
        CHECK(!book_manager.AddBook(book));

        book.author = "Author"s;
        book.isbn = "1"s;
        CHECK(book_manager.AddBook(book)); // first time should return true
        CHECK(!book_manager.AddBook(book)); // second is false

        book.isbn = "14 SymbolsISBN"s; // max == 13
        CHECK(!book_manager.AddBook(book));

        book.isbn = "2"s;
        CHECK(book_manager.AddBook(book)); // same book with another ISBN is true

        book.isbn = std::nullopt; // NULL
        CHECK(book_manager.AddBook(book)); // true because NULL ISBN is possible
    }

    SECTION("call GetAllBooks method. It should sort book info") {
        SECTION("when year is different, method should return books in descending order") {
            BookInfo book1{.title = "A", .author = "A"s, .year = 2023};
            book_manager.AddBook(book1);
            BookInfo book2{.title = "A", .author = "A"s, .year = 2024};
            book_manager.AddBook(book2);
            
            //It can be implimented with matchers but next time)))
            auto res = book_manager.GetAllBooks();
            CHECK(res[0].year == 2024);
        }

        SECTION("when year is the same, sort by title") {
            BookInfo book1{.title = "B", .author = "A"s, .year = 2024};
            book_manager.AddBook(book1);
            BookInfo book2{.title = "A", .author = "A"s, .year = 2024};
            book_manager.AddBook(book2);

            auto res = book_manager.GetAllBooks();
            CHECK(*res[0].title == "A"s);
        }

        SECTION("when year and title are the same, sort by author") {
            BookInfo book1{.title = "A", .author = "B"s, .year = 2024};
            book_manager.AddBook(book1);
            BookInfo book2{.title = "A", .author = "A"s, .year = 2024};
            book_manager.AddBook(book2);

            auto res = book_manager.GetAllBooks();
            CHECK(*res[0].author == "A"s);
        }

        SECTION("when year, title, and author are the same, sort by ISBN") {
            BookInfo book1{.title = "A", .author = "A"s, .year = 2024, .isbn = "BBBBBBBBBBBBB"s};
            book_manager.AddBook(book1);
            BookInfo book2{.title = "A", .author = "A"s, .year = 2024, .isbn = "AAAAAAAAAAAAA"s};
            book_manager.AddBook(book2);

            auto res = book_manager.GetAllBooks();
            CHECK(*res[0].isbn == "AAAAAAAAAAAAA"s);
        }
    }
}
