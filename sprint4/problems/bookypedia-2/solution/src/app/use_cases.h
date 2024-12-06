#pragma once

#include "../domain/author_fwd.h"
#include "../domain/book_fwd.h"

#include <string>
#include <vector>

namespace app {

class UseCases {
public:
    virtual domain::AuthorId AddAuthor(const std::string& name) = 0;
    virtual domain::BookId AddBook(const std::string& author_id, const std::string& title, int publication_year) = 0;
    virtual void AddTag(const std::string& book_id, const std::string& tag) = 0;

    virtual void DeleteAuthor(const std::string& author_id) = 0;
    virtual void DeleteBook(const std::string& book_id) = 0;
    virtual void DeleteBookTags(const std::string& book_id) = 0;

    virtual void EditAuthor(const std::string& author_id, const std::string& new_author_name) = 0;
    virtual void EditBook(const std::string& book_id, const std::string& author_id, const std::string& new_book_title, int new_publication_year) = 0;

    virtual std::vector<domain::Author> GetAuthors() = 0;
    virtual std::vector<domain::Book> GetAllBooks() = 0;
    virtual std::vector<std::string> GetBookTags(const std::string& book_id) = 0;
    virtual std::vector<domain::Book> GetAuthorBooks(const std::string& autor_id) = 0;
    virtual std::optional<domain::Author> GetAuthorByName(const std::string& name) = 0;
    virtual std::optional<domain::Author> GetAuthorById(const std::string& author_id) = 0;
    virtual std::vector<domain::Book> GetBooksByTitle(const std::string& title) = 0;
    virtual std::optional<domain::Book> GetBookById(const std::string& book_id) = 0;


    virtual void StartTransaction() = 0;
    virtual void Commit() = 0;

protected:
    ~UseCases() = default;
};

}  // namespace app
