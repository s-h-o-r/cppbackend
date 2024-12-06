#pragma once

#include "../domain/author_fwd.h"
#include "../domain/book_fwd.h"

#include <memory>

namespace app {

class UnitOfWork {
public:
    virtual ~UnitOfWork() = default;
    virtual void Commit() = 0;
    virtual void Abort() = 0;
    virtual domain::AuthorRepository& Authors() = 0;
    virtual domain::BookRepository& Books() = 0;
    virtual domain::TagRepository& Tags() = 0;
};

class UnitOfWorkFactory {
public:

    virtual std::unique_ptr<UnitOfWork> CreateUnitOfWork() = 0;

protected:
    ~UnitOfWorkFactory() = default;
};

} // namespace app
