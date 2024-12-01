#include "view.h"

#include <algorithm>
#include <boost/algorithm/string/trim.hpp>
#include <cassert>
#include <iostream>

#include "../domain/author.h"
#include "../domain/book.h"
#include "../app/use_cases.h"
#include "../menu/menu.h"

using namespace std::literals;
namespace ph = std::placeholders;

namespace ui {
namespace detail {

std::ostream& operator<<(std::ostream& out, const AuthorInfo& author) {
    out << author.name;
    return out;
}

std::ostream& operator<<(std::ostream& out, const BookInfo& book) {
    out << book.title << ", " << book.publication_year;
    return out;
}

}  // namespace detail

template <typename T>
void PrintVector(std::ostream& out, const std::vector<T>& vector) {
    int i = 1;
    for (auto& value : vector) {
        out << i++ << " " << value << std::endl;
    }
}

View::View(menu::Menu& menu, app::UseCases& use_cases, std::istream& input, std::ostream& output)
    : menu_{menu}
    , use_cases_{use_cases}
    , input_{input}
    , output_{output} {
    auto lambda = [this](std::istream& cmd_input) {
        return this->AddAuthor(cmd_input);
    };
    menu_.AddAction(  //
        "AddAuthor"s, "name"s, "Adds author"s, lambda
        // либо
        // [this](auto& cmd_input) { return AddAuthor(cmd_input); }
    );
    menu_.AddAction("AddBook"s, "<pub year> <title>"s, "Adds book"s,
                    std::bind(&View::AddBook, this, ph::_1));
    menu_.AddAction("ShowAuthors"s, {}, "Show authors"s, std::bind(&View::ShowAuthors, this));
    menu_.AddAction("ShowBooks"s, {}, "Show books"s, std::bind(&View::ShowBooks, this));
    menu_.AddAction("ShowAuthorBooks"s, {}, "Show author books"s,
                    std::bind(&View::ShowAuthorBooks, this));
}

bool View::AddAuthor(std::istream& cmd_input) const {
    try {
        std::string name;
        std::getline(cmd_input, name);
        use_cases_.StartTransaction();
        AddAuthor(name);
        use_cases_.Commit();
    } catch (const std::exception&) {
        output_ << "Failed to add author"sv << std::endl;
    }
    return true;
}

domain::AuthorId View::AddAuthor(std::string name) const {
        boost::algorithm::trim(name);

        if (name.empty()) {
            throw std::runtime_error("Empty author name");
        }
        return use_cases_.AddAuthor(std::move(name));
}

bool View::AddBook(std::istream& cmd_input) const {
    try {
        use_cases_.StartTransaction();
        if (auto params = GetBookParams(cmd_input)) {
            if (params->title.empty()) {
                throw std::runtime_error("Empty book title");
            }
            auto book_id = use_cases_.AddBook(params->author_id, params->title, params->publication_year);
            for (const auto& tag : params->tags) {
                use_cases_.AddTag(book_id.ToString(), tag);
            }
            use_cases_.Commit();
        }
    } catch (const std::exception&) {
        output_ << "Failed to add book"sv << std::endl;
    }
    return true;
}

bool View::DeleteAuthor(std::istream& cmd_input) const {
    try {
        use_cases_.StartTransaction();
        std::string author_name;
        std::getline(cmd_input, author_name)
        boost::algorithm::trim(author_name);

        std::optional<std::string> author_id;

        if (author_name.empty()) {
            auto author_id = SelectAuthor();
        } else {
            auto author = use_cases_.GetAuthor(author_name);
            if (!author) {
                throw std::runtime_error("Unknown author to delete");
            }
            author_id = author->id_.ToString();
        }

        if (author_id) {
            DeleteAuthorBooks(author_id);
            use_cases_.DeleteAuthor(author_id);
            use_cases_.Commit()
        }
    } catch (const std::exception&) {
        output_ << "Failed to delete author"sv << std::endl;
    }
    return true;
}

bool View::ShowAuthors() const {
    PrintVector(output_, GetAuthors());
    return true;
}

bool View::ShowBooks() const {
    PrintVector(output_, GetBooks());
    return true;
}

bool View::ShowAuthorBooks() const {
    // TODO: handle error
    try {
        if (auto author_id = SelectAuthor()) {
            PrintVector(output_, GetAuthorBooks(*author_id));
        }
    } catch (const std::exception&) {
        throw std::runtime_error("Failed to Show Books");
    }
    return true;
}

std::optional<detail::AddBookParams> View::GetBookParams(std::istream& cmd_input) const {
    detail::AddBookParams params;

    cmd_input >> params.publication_year;
    std::getline(cmd_input, params.title);
    boost::algorithm::trim(params.title);

    output_ << "Enter author name or empty line to select from list:"sv << std::endl;
    std::string author_name;
    if (std::getline(input_, author_name) || !author_name.empty()) {
        boost::algorithm::trim(author_name);
        auto author = use_cases_.GetAuthor(author_name);
        if (!author) {
            output_ << "No author found. Do you want to add Jack London (y/n)?"sv << std::endl;
            std::string answer;
            if (!std::getline(input_, answer) || answer.empty() || answer.size() > 1 || std::tolower(answer[0]) != 'y') {
                throw std::runtime_error("Cannot identify book's author");
            }
            auto author_id = AddAuthor(author_name);
            params.author_id = author_id.ToString();
        }
    } else {
        auto author_id = SelectAuthor();
        if (not author_id.has_value())
            return std::nullopt;
        else {
            params.author_id = author_id.value();
        }
    }

    output_ << "Enter tags (comma separated):"sv << std::endl;
    params.tags = GetBookTags(cmd_input);
    return params;
}

std::optional<std::string> View::SelectAuthor() const {
    output_ << "Select author:" << std::endl;
    auto authors = GetAuthors();
    PrintVector(output_, authors);
    output_ << "Enter author # or empty line to cancel" << std::endl;

    std::string str;
    if (!std::getline(input_, str) || str.empty()) {
        return std::nullopt;
    }

    int author_idx;
    try {
        author_idx = std::stoi(str);
    } catch (std::exception const&) {
        throw std::runtime_error("Invalid author num");
    }

    --author_idx;
    if (author_idx < 0 or author_idx >= authors.size()) {
        throw std::runtime_error("Invalid author num");
    }

    return authors[author_idx].id;
}

std::vector<detail::AuthorInfo> View::GetAuthors() const {
    std::vector<detail::AuthorInfo> dst_autors;

    for (const auto& author : use_cases_.GetAuthors()) {
        dst_autors.push_back({author.GetId().ToString(), author.GetName()});
    }
    return dst_autors;
}

std::vector<detail::BookInfo> View::GetBooks() const {
    std::vector<detail::BookInfo> books;

    for (const auto& book : use_cases_.GetAllBooks()) {
        books.push_back({book.book_id_.ToString(), book.GetTitle(), book.GetPublicationYear()});
    }

    return books;
}

std::vector<detail::BookInfo> View::GetAuthorBooks(const std::string& author_id) const {
    std::vector<detail::BookInfo> books;

    for (const auto& book : use_cases_.GetAuthorBooks(author_id)) {
        books.push_back({book.book_id_.ToString(), book.GetTitle(), book.GetPublicationYear()});
    }

    return books;
}

std::vector<std::string> View::GetBookTags(std::istream& cmd_input) const {
    std::vector<std::string> tags;
    std::string tag;
    while (std::getline(cmd_input, tag, ',')) {
        boost::algorithm::trim(tag);
        if (!tag.empty() && std::find(tags.begin(), tags.end(), tag) == tags.end()) {
            tags.push_back(tag);
        }
    }
    return tags;
}

void View::DeleteTags(const std::string& book_id) const {
    use_cases_.DeleteBookTags(book_id);
}

void View::DeleteAuthorBooks(const std::string& author_id) const {
    auto books = GetAuthorBooks(author_id);

    for (const auto& book_info : books) {
        use_cases_.DeleteBook(book_info.id);
        DeleteTags(book_info.id);
    }
}

}  // namespace ui
