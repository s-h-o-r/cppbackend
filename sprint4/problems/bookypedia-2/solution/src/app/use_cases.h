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

    virtual std::vector<domain::Author> GetAuthors() = 0;
    virtual std::vector<domain::Book> GetAllBooks() = 0;
    virtual std::vector<domain::Book> GetAuthorBooks(const std::string& autor_id) = 0;
    virtual std::optional<domain::Author> GetAuthor(const std::string& name) = 0; 

    virtual void StartTransaction() = 0;
    virtual void Commit() = 0;

protected:
    ~UseCases() = default;
};

}  // namespace app
