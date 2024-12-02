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

struct BookInfoWithAuthor {
    std::string id;
    std::string title;
    int publication_year;
    std::string author_name;
};

struct FullBookInfo {
    std::string title;
    int publication_year;
    std::string author_name;
    std::vector<std::string> tags;
};

}  // namespace detail

class View {
public:
    View(menu::Menu& menu, app::UseCases& use_cases, std::istream& input, std::ostream& output);

private:
    bool AddAuthor(std::istream& cmd_input) const;
    bool AddBook(std::istream& cmd_input) const;

    bool DeleteAuthor(std::istream& cmd_input) const;
    bool DeleteBook(std::istream& cmd_input) const;

    bool EditAuthor(std::istream& cmd_input) const;
    bool EditBook(std::istream& cmd_input) const;

    bool ShowAuthors() const;
    bool ShowBooks() const;
    bool ShowAuthorBooks() const;
    bool ShowBook(std::istream& cmd_input) const;

    menu::Menu& menu_;
    app::UseCases& use_cases_;
    std::istream& input_;
    std::ostream& output_;

private:
    domain::AuthorId AddAuthor(std::string name) const;
    std::optional<detail::AddBookParams> GetBookParams(std::istream& cmd_input) const;
    detail::AddBookParams GetBookParamsForEdit(const std::string& book_id) const;
    std::optional<std::string> SelectAuthor() const;
    std::optional<std::string> SelectBook() const;
    std::optional<std::string> GetOrSelectAuthor(std::istream& cmd_input) const;
    std::optional<std::string> GetOrSelectBook(std::istream& cmd_input) const;
    std::vector<detail::AuthorInfo> GetAuthors() const;
    std::vector<detail::BookInfoWithAuthor> GetBooks() const;
    std::optional<detail::FullBookInfo> GetBook(const std::string& book_id) const;
    std::vector<detail::BookInfoWithAuthor> GetBooksByTitle(const std::string& title) const;
    std::vector<detail::BookInfo> GetAuthorBooks(const std::string& author_id) const;
    std::vector<std::string> GetBookTags() const;
    void DeleteTags(const std::string& book_id) const;
    void DeleteAuthorBooks(const std::string& author_id) const;
    
};

}  // namespace ui
