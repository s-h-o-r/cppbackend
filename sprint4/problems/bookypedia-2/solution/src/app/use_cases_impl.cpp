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

std::vector<domain::Author> UseCasesImpl::GetAuthors() {
    CheckTransaction();
    return transaction_->Authors().GetAuthors();
}

std::vector<domain::Book> UseCasesImpl::GetAllBooks() {
    CheckTransaction();
    return transaction_->Books().GetAllBooks();
}

std::vector<domain::Book> UseCasesImpl::GetAuthorBooks(const std::string& author_id) {
    CheckTransaction();
    return transaction_->Books().GetAuthorBooks(AuthorId::FromString(author_id));
}

std::optional<domain::Author> UseCasesImpl::GetAuthor(const std::string& name) {
    CheckTransaction();
    return transaction_->Authors().GetAuthor(name);
}

void UseCasesImpl::StartTransaction() {
    transaction_.reset();
    transaction_ = db_.MakeTransaction();
}

void UseCasesImpl::Commit() {
    CheckTransaction();
    transaction_->Commit();
}

}  // namespace app
