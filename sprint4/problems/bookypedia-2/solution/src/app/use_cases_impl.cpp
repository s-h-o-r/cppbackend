#include "use_cases_impl.h"

#include "../domain/author.h"
#include "../domain/book.h"

namespace app {
using namespace domain;

void UseCasesImpl::AddAuthor(const std::string& name) {
    auto transaction = db_.MakeTransaction();
    transaction->Authors().Save({AuthorId::New(), name});
    transaction->Commit();
}

std::vector<domain::Author> UseCasesImpl::GetAuthors() {
    return db_.MakeTransaction()->Authors().GetAuthors();
}

void UseCasesImpl::AddBook(const std::string& author_id, const std::string& title, int publication_year) {
    auto transaction = db_.MakeTransaction();
    transaction->Books().Save({BookId::New(), AuthorId::FromString(author_id), title, publication_year});
    transaction->Commit();
}

std::vector<domain::Book> UseCasesImpl::GetAllBooks() {
    return db_.MakeTransaction()->Books().GetAllBooks();
}

std::vector<domain::Book> UseCasesImpl::GetAuthorBooks(const std::string& author_id) {
    return db_.MakeTransaction()->Books().GetAuthorBooks(AuthorId::FromString(author_id));
}

}  // namespace app
