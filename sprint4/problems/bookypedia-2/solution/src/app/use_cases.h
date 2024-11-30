#pragma once

#include "../domain/author_fwd.h"
#include "../domain/book_fwd.h"

#include <string>
#include <vector>

namespace app {

class UseCases {
public:
    virtual void AddAuthor(const std::string& name) = 0;
    virtual std::vector<domain::Author> GetAuthors() = 0;
    virtual void AddBook(const std::string& author_id, const std::string& title, int publication_year) = 0;
    virtual std::vector<domain::Book> GetAllBooks() = 0;
    virtual std::vector<domain::Book> GetAuthorBooks(const std::string& autor_id) = 0;
    
protected:
    ~UseCases() = default;
};

}  // namespace app
