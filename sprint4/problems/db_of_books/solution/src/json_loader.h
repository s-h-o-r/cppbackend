#pragma once

#include <optional>
#include <string>
#include <vector>

#include <boost/json.hpp>

#include "book_manager.h"

namespace json_loader {

struct Query {
    std::string name;
    std::optional<book_manager::BookInfo> payload;
};

Query ParseQuery(std::string query);
std::string PrepareResultAddBook(bool action_result);
std::string PrepareResultAllBooks(const std::vector<book_manager::BookInfo>& books);

} // namespace json_loader
