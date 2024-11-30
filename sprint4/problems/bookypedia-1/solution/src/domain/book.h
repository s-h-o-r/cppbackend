#pragma once
#include <string>
#include <vector>

#include "../util/tagged_uuid.h"
#include "author.h"

namespace domain {

namespace detail {
struct BookTag {};
}  // namespace detail

using BookId = util::TaggedUUID<detail::BookTag>;

class Book {
public:
    Book(BookId book_id, AuthorId author_id, std::string title, std::uint16_t publication_year)
        : book_id_(book_id)
        , author_id_(author_id)
        , title_(title)
        , publication_year_(publication_year) {
    }

    const BookId& GetBookId() const noexcept {
        return book_id_;
    }

    const AuthorId& GetAuthorId() const noexcept {
        return author_id_;
    }

    const std::string& GetTitle() const noexcept {
        return title_;
    }

    std::uint16_t GetPublicationYear() const noexcept {
        return publication_year_;
    }

private:
    BookId book_id_;
    AuthorId author_id_;
    std::string title_;
    std::uint16_t publication_year_;
};

class BookRepository {
public:
    virtual void Save(const Book& book) = 0;
    virtual std::vector<Book> GetAllBooks() = 0;
    virtual std::vector<Book> GetAuthorBooks(const AuthorId& author_id) = 0;

protected:
    ~BookRepository() = default;
};

}  // namespace domain
