#include "use_cases_impl.h"

#include "../domain/author.h"
#include "../domain/book.h"

namespace app {
using namespace domain;

domain::AuthorId UseCasesImpl::AddAuthor(const std::string& name) {
    CheckTransaction();
    auto author_id = AuthorId::New();
    transaction_->Authors().Save({author_id, name});
    return author_id;
}

domain::BookId UseCasesImpl::AddBook(const std::string& author_id, const std::string& title, int publication_year) {
    CheckTransaction();
    auto book_id = BookId::New();
    transaction_->Books().Save(domain::Book{book_id, AuthorId::FromString(author_id), title, publication_year});
    return book_id;
}

void UseCasesImpl::AddTag(const std::string& book_id, const std::string& tag) {
    CheckTransaction();
    transaction_->Tags().Save(domain::BookId::FromString(book_id), tag);
}

void UseCasesImpl::DeleteAuthor(const std::string& author_id) {
    CheckTransaction();
    transaction_->Authors().Delete(domain::AuthorId::FromString(author_id));
}

void UseCasesImpl::DeleteBook(const std::string& book_id) {
    CheckTransaction();
    transaction_->Books().Delete(domain::BookId::FromString(book_id));
}

void UseCasesImpl::DeleteBookTags(const std::string& book_id) {
    CheckTransaction();
    transaction_->Tags().Delete(domain::BookId::FromString(book_id));
}

void UseCasesImpl::EditAuthor(const std::string& author_id, const std::string& new_author_name) {
    CheckTransaction();
    transaction_->Authors().Edit(domain::Author(domain::AuthorId::FromString(author_id), new_author_name));
}

void UseCasesImpl::EditBook(const std::string& book_id, const std::string& author_id, const std::string& new_book_title, int new_publication_year) {
    CheckTransaction();
    transaction_->Books().Edit(domain::Book{domain::BookId::FromString(book_id), domain::AuthorId::FromString(author_id),
        new_book_title, new_publication_year});
}

std::vector<domain::Author> UseCasesImpl::GetAuthors() {
    CheckTransaction();
    return transaction_->Authors().GetAuthors();
}

std::vector<domain::Book> UseCasesImpl::GetAllBooks() {
    CheckTransaction();
    return transaction_->Books().GetAllBooks();
}

std::vector<std::string> UseCasesImpl::GetBookTags(const std::string& book_id) {
    CheckTransaction();
    return transaction_->Tags().GetBookTags(domain::BookId::FromString(book_id));
}

std::vector<domain::Book> UseCasesImpl::GetAuthorBooks(const std::string& author_id) {
    CheckTransaction();
    return transaction_->Books().GetAuthorBooks(AuthorId::FromString(author_id));
}

std::optional<domain::Author> UseCasesImpl::GetAuthorByName(const std::string& name) {
    CheckTransaction();
    return transaction_->Authors().GetAuthorByName(name);
}

std::optional<domain::Author> UseCasesImpl::GetAuthorById(const std::string& author_id) {
    CheckTransaction();
    return transaction_->Authors().GetAuthorById(domain::AuthorId::FromString(author_id));
}

std::vector<domain::Book> UseCasesImpl::GetBooksByTitle(const std::string& title) {
    CheckTransaction();
    return transaction_->Books().GetBooksByTitle(title);
}

std::optional<domain::Book> UseCasesImpl::GetBookById(const std::string& book_id) {
    CheckTransaction();
    return transaction_->Books().GetBookById(domain::BookId::FromString(book_id));
}

void UseCasesImpl::StartTransaction() {
    transaction_.reset();
    transaction_ = db_.MakeTransaction();
}

void UseCasesImpl::Commit() {
    CheckTransaction();
    try {
        transaction_->Commit();
    } catch (const std::exception& e) {
        transaction_->Abort();
        throw e;
    }
}

}  // namespace app
