#pragma once
#include "../postgres/postgres.h"
#include "use_cases.h"

#include <string>
#include <vector>

namespace app {

class UseCasesImpl : public UseCases {
public:
    explicit UseCasesImpl(postgres::Database& db)
        : db_(db) {
    }

    void AddAuthor(const std::string& name) override;
    std::vector<domain::Author> GetAuthors() override;
    void AddBook(const std::string& author_id, const std::string& title, int publication_year) override;
    std::vector<domain::Book> GetAllBooks() override;
    std::vector<domain::Book> GetAuthorBooks(const std::string& author_id) override;
private:
    postgres::Database& db_;
};

}  // namespace app
