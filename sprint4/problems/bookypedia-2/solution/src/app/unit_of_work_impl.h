#pragma once

#include "unit_of_work.h"

#include <pqxx/pqxx>

namespace posgres {

class UnitOfWorkImpl : public app::UnitOfWork {
public:
    UnitOfWorkImpl(pqxx::connection& conn)
    : work(conn) {
    }

    void Commit() override;
    domain::AuthorRepository& Authors() override;
    domain::BookRepository& Books() override;

private:
    pqxx::work work;
    
};

class UnitOfWorkFactoryImpl : public app::UnitOfWorkFactory {
public:
    std::unique_ptr<ap::UnitOfWork> CreateUnitOfWork() override;
};

}
