#pragma once
#include <iosfwd>
#include <optional>
#include <string>
#include <vector>

#include "../domain/author.h"

namespace menu {
class Menu;
}

namespace app {
class UseCases;
}

namespace ui {
namespace detail {

struct AddBookParams {
    std::string title;
    std::string author_id;
    int publication_year = 0;
    std::vector<std::string> tags;
};

struct AuthorInfo {
    std::string id;
    std::string name;
};

struct BookInfo {
    std::string id;
    std::string title;
    int publication_year;
};

}  // namespace detail

class View {
public:
    View(menu::Menu& menu, app::UseCases& use_cases, std::istream& input, std::ostream& output);

private:
    bool AddAuthor(std::istream& cmd_input) const;
    domain::AuthorId AddAuthor(std::string name) const;
    bool AddBook(std::istream& cmd_input) const;

    bool DeleteAuthor() const;
    //bool DeleteBook() const;

    bool ShowAuthors() const;
    bool ShowBooks() const;
    bool ShowAuthorBooks() const;

    std::optional<detail::AddBookParams> GetBookParams(std::istream& cmd_input) const;
    std::optional<std::string> SelectAuthor() const;
    std::vector<detail::AuthorInfo> GetAuthors() const;
    std::vector<detail::BookInfo> GetBooks() const;
    std::vector<detail::BookInfo> GetAuthorBooks(const std::string& author_id) const;
    std::vector<std::string> GetBookTags(std::istream& cmd_input) const;
    void DeleteTags(const std::string& book_id) const;
    void DeleteAuthorBooks(const std::string& author_id) const;

    menu::Menu& menu_;
    app::UseCases& use_cases_;
    std::istream& input_;
    std::ostream& output_;
    
};

}  // namespace ui
