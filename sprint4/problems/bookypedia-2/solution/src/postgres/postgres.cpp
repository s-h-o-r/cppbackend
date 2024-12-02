#include "postgres.h"

#include <pqxx/pqxx>
#include <pqxx/zview.hxx>

#include <string>
#include <tuple>

namespace postgres {

using namespace std::literals;
using pqxx::operator"" _zv;

void AuthorRepositoryImpl::Save(const domain::Author& author) {
    // Пока каждое обращение к репозиторию выполняется внутри отдельной транзакции
    // В будущих уроках вы узнаете про паттерн Unit of Work, при помощи которого сможете несколько
    // запросов выполнить в рамках одной транзакции.
    // Вы также может самостоятельно почитать информацию про этот паттерн и применить его здесь.
    work_.exec_params(
        R"(
INSERT INTO authors (id, name) VALUES ($1, $2)
ON CONFLICT (id) DO UPDATE SET name=$2;
)"_zv,
        author.GetId().ToString(), author.GetName());
}

std::vector<domain::Author> AuthorRepositoryImpl::GetAuthors() {
    auto query_text = "SELECT id, name FROM authors ORDER BY name"_zv;

    std::vector<domain::Author> authors;
    for (auto [id, name] : work_.query<std::string, std::string>(query_text)) {
        authors.push_back({domain::AuthorId::FromString(id), name});
    }
    return authors;
}

std::optional<domain::Author> AuthorRepositoryImpl::GetAuthorByName(const std::string& name) {
    auto query_text = "SELECT id, name FROM authors WHERE name = $1"_zv;

    if (std::optional res = work_.query01<std::string, std::string>(query_text, name)) {
        return domain::Author{domain::AuthorId::FromString(std::get<0>(*res)), std::get<1>(*res)};
    }
    return std::nullopt;
}

std::optional<domain::Author> AuthorRepositoryImpl::GetAuthorById(const domain::AuthorId& author_id) {
    auto query_text = "SELECT id, name FROM authors WHERE id = $1"_zv;

    if (std::optional res = work_.query01<std::string, std::string>(query_text, author_id.ToString())) {
        return domain::Author{domain::AuthorId::FromString(std::get<0>(*res)), std::get<1>(*res)};
    }
    return std::nullopt;
}

void AuthorRepositoryImpl::Delete(const domain::AuthorId& author_id) {
    work_.exec_params("DELETE FROM authors WHERE id = $1"_zv, author_id.ToString());
}

void AuthorRepositoryImpl::Edit(const domain::Author& edited_author) {
    work_.exec_params("UPDATE authors SET name = $2 WHERE id = $1"_zv, edited_author.GetId().ToString(), edited_author.GetName());
}

void BookRepositoryImpl::Save(const domain::Book& book) {
    work_.exec_params(R"(
INSERT INTO books (id, author_id, title, publication_year) VALUES ($1, $2, $3, $4)
ON CONFLICT (id) DO UPDATE SET author_id=$2, title=$3, publication_year=$4;
)"_zv,
        book.GetBookId().ToString(), book.GetAuthorId().ToString(), book.GetTitle(), book.GetPublicationYear());
}

std::vector<domain::Book> BookRepositoryImpl::GetAllBooks() {
    auto query_text = R"(
SELECT book.id, book.author_id, book.title, book.publication_year 
FROM books book
JOIN authors author ON book.author_id = author.id
ORDER BY book.title, author.name, book.publication_year
)"_zv;

    std::vector<domain::Book> books;
    for (auto [book_id, author_id, title, publication_year]
         : work_.query<std::string, std::string, std::string, int>(query_text)) {
        books.push_back({domain::BookId::FromString(book_id), domain::AuthorId::FromString(author_id),
            title, publication_year});
    }
    return books;
}

std::vector<domain::Book> BookRepositoryImpl::GetAuthorBooks(const domain::AuthorId& author_id) {
    auto query_text = "SELECT id, author_id, title, publication_year FROM books WHERE author_id = $1 ORDER BY publication_year, title"_zv;

    std::vector<domain::Book> books;
    for (auto [book_id, author_id, title, publication_year]
         : work_.query<std::string, std::string, std::string, int>(query_text, author_id.ToString())) {
        books.push_back({domain::BookId::FromString(book_id), domain::AuthorId::FromString(author_id),
            title, publication_year});
    }
    return books;
}

void BookRepositoryImpl::Delete(const domain::BookId& book_id) {
    work_.exec_params("DELETE FROM books WHERE id = $1"_zv, book_id.ToString());
}

std::vector<domain::Book> BookRepositoryImpl::GetBooksByTitle(const std::string& title) {
    auto query_text = "SELECT id, author_id, title, publication_year FROM books WHERE title = $1"_zv;

    std::vector<domain::Book> books;

    for (auto [book_id, author_id, title, publication_year] :
         work_.query<std::string, std::string, std::string, int>(query_text, title)) {
        books.push_back(domain::Book{domain::BookId::FromString(book_id), domain::AuthorId::FromString(author_id),
            title, publication_year});
    }
    return books;
}

std::optional<domain::Book> BookRepositoryImpl::GetBookById(const domain::BookId& book_id) {
    auto query_text = "SELECT id, author_id, title, publication_year FROM books WHERE id = $1"_zv;

    if (std::optional res = work_.query01<std::string, std::string, std::string, int>(query_text, book_id.ToString())) {
        return domain::Book{domain::BookId::FromString(std::get<0>(*res)), domain::AuthorId::FromString(std::get<1>(*res)),
                            std::get<2>(*res), std::get<3>(*res)};
    }
    return std::nullopt;
}

void BookRepositoryImpl::Edit(const domain::Book& edited_book) {
    work_.exec_params("UPDATE books SET title = $2, publication_year = $3 WHERE id = $1"_zv,
                      edited_book.GetBookId().ToString(), edited_book.GetTitle(), edited_book.GetPublicationYear());
}

void TagRepositoryImpl::Save(domain::BookId book_id, std::string tag) {
    work_.exec_params("INSERT INTO book_tags (book_id, tag) VALUES ($1, $2)"_zv, book_id.ToString(), tag);
}

void TagRepositoryImpl::Delete(const domain::BookId& book_id) {
    work_.exec_params("DELETE FROM book_tags WHERE book_id = $1"_zv, book_id.ToString());
}

std::vector<std::string> TagRepositoryImpl::GetBookTags(const domain::BookId& book_id) {
    std::vector<std::string> tags;

    for (auto [tag] : work_.query<std::string>("SELECT tag FROM book_tags WHERE book_id = $1"_zv, book_id.ToString())) {
        tags.push_back(tag);
    }
    return tags;
}

Database::Database(pqxx::connection connection)
    : connection_{std::move(connection)}
    , unit_of_work_factory_(connection_) {
    pqxx::work work{connection_};
    work.exec(R"(
CREATE TABLE IF NOT EXISTS authors (
    id UUID CONSTRAINT author_id_constraint PRIMARY KEY,
    name varchar(100) UNIQUE NOT NULL
);
)"_zv);

    work.exec(R"(
CREATE TABLE IF NOT EXISTS books (
    id UUID CONSTRAINT books_id_constraint PRIMARY KEY,
    author_id UUID NOT NULL,
    title varchar(100) NOT NULL,
    publication_year INTEGER
);
)"_zv);

    work.exec(R"(
CREATE TABLE IF NOT EXISTS book_tags (
    book_id UUID,
    tag varchar(30) NOT NULL
);
)"_zv);
    work.commit();
}

}  // namespace postgres
