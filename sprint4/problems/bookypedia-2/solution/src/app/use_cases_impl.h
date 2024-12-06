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

    domain::AuthorId AddAuthor(const std::string& name) override;
    domain::BookId AddBook(const std::string& author_id, const std::string& title, int publication_year) override;
    void AddTag(const std::string& book_id, const std::string& tag) override;

    void DeleteAuthor(const std::string& author_id) override;
    void DeleteBook(const std::string& book_id) override;
    void DeleteBookTags(const std::string& book_id) override;

    void EditAuthor(const std::string& author_id, const std::string& new_author_name) override;
    void EditBook(const std::string& book_id, const std::string& author_id, const std::string& new_book_title, int new_publication_year) override;

    std::vector<domain::Author> GetAuthors() override;
    std::vector<domain::Book> GetAllBooks() override;
    std::vector<std::string> GetBookTags(const std::string& book_id) override;
    std::vector<domain::Book> GetAuthorBooks(const std::string& author_id) override;
    std::optional<domain::Author> GetAuthorByName(const std::string& name) override;
    std::optional<domain::Author> GetAuthorById(const std::string& author_id) override;
    std::vector<domain::Book> GetBooksByTitle(const std::string& title) override;
    std::optional<domain::Book> GetBookById(const std::string& book_id) override;

    void StartTransaction() override;
    void Commit() override;

private:
    postgres::Database& db_;
    std::unique_ptr<UnitOfWork> transaction_{nullptr};

    void NullTransactionError() {
        throw std::logic_error("Transaction is closed. Before commite changes open a transaction");
    }

    void CheckTransaction() {
        if (transaction_ == nullptr) {
            NullTransactionError();
        }
    }
};

}  // namespace app
