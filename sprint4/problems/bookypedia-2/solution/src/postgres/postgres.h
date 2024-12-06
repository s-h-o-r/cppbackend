#pragma once
#include <pqxx/connection>
#include <pqxx/transaction>

#include "../app/unit_of_work.h"
#include "../domain/author.h"
#include "../domain/book.h"

namespace postgres {

class AuthorRepositoryImpl : public domain::AuthorRepository {
public:
    explicit AuthorRepositoryImpl(pqxx::work& work)
        : work_{work} {
    }

    void Save(const domain::Author& author) override;
    std::vector<domain::Author> GetAuthors() override;
    std::optional<domain::Author> GetAuthorByName(const std::string& name) override;
    std::optional<domain::Author> GetAuthorById(const domain::AuthorId& author_id) override;
    void Delete(const domain::AuthorId& author_id) override;
    void Edit(const domain::Author& edited_author) override;
private:
    pqxx::work& work_;
};

class BookRepositoryImpl : public domain::BookRepository {
public:
    explicit BookRepositoryImpl(pqxx::work& work)
        : work_(work) {
    }

    void Save(const domain::Book& book) override;
    std::vector<domain::Book> GetAllBooks() override;
    std::vector<domain::Book> GetAuthorBooks(const domain::AuthorId& author_id) override;
    void Delete(const domain::BookId& book_id) override;
    std::vector<domain::Book> GetBooksByTitle(const std::string& title) override;
    std::optional<domain::Book> GetBookById(const domain::BookId& book_id) override;
    void Edit(const domain::Book& edited_book) override;

private:
    pqxx::work& work_;
};

class TagRepositoryImpl : public domain::TagRepository {
public:
    explicit TagRepositoryImpl(pqxx::work& work)
        : work_(work) {
    }

    void Save(domain::BookId book_id, std::string tag) override;
    void Delete(const domain::BookId& book_id) override;
    std::vector<std::string> GetBookTags(const domain::BookId& book_id) override;

private:
    pqxx::work& work_;
};

class UnitOfWorkImpl : public app::UnitOfWork {
public:
    UnitOfWorkImpl(pqxx::connection& conn)
    : work_(conn)
    , authors_(work_)
    , books_(work_)
    , book_tags_(work_) {
    }

    void Commit() override {
        work_.commit();
    }

    void Abort() override {
        work_.abort();
    }

    domain::AuthorRepository& Authors() override {
        return authors_;
    }

    domain::BookRepository& Books() override {
        return books_;
    }

    domain::TagRepository& Tags() override {
        return book_tags_;
    }

private:
    pqxx::work work_;
    AuthorRepositoryImpl authors_;
    BookRepositoryImpl books_;
    TagRepositoryImpl book_tags_;
};

class UnitOfWorkFactoryImpl : public app::UnitOfWorkFactory {
public:
    explicit UnitOfWorkFactoryImpl(pqxx::connection& conn)
    : connection_(conn) {
    }

    std::unique_ptr<app::UnitOfWork> CreateUnitOfWork() override {
        return std::make_unique<UnitOfWorkImpl>(connection_);
    }

private:
    pqxx::connection& connection_;
};

class Database {
public:
    explicit Database(pqxx::connection connection);

    std::unique_ptr<app::UnitOfWork> MakeTransaction() {
        return unit_of_work_factory_.CreateUnitOfWork();
    }

private:
    pqxx::connection connection_;
    UnitOfWorkFactoryImpl unit_of_work_factory_;
};

}  // namespace postgres
