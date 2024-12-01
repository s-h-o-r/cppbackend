#pragma once

#include "../domain/author_fwd.h"
#include "../domain/book_fwd.h"

#include <memory>

namespace app {

class UnitOfWork {
public:
    virtual void Commit() = 0;
    virtual domain::AuthorRepository& Authors() = 0;
    virtual domain::BookRepository& Books() = 0;

protected:
    ~UnitOfWork() = default;
};

class UnitOfWorkFactory {
public:

    virtual std::unique_ptr<UnitOfWork> CreateUnitOfWork() = 0;

protected:
    ~UnitOfWorkFactory() = default;
};

} // namespace app
