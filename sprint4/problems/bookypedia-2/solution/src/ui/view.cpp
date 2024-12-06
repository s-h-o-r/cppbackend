#include "view.h"

#include <algorithm>
#include <boost/algorithm/string/trim_all.hpp>
#include <cassert>
#include <iostream>
#include <string>

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

std::ostream& operator<<(std::ostream& out, const BookInfoWithAuthor& book) {
    out << book.title << " by " << book.author_name << ", " << book.publication_year;
    return out;
}

std::ostream& operator<<(std::ostream& out, const FullBookInfo& book) {
    out << "Title: " << book.title << "\n"
    << "Author: " << book.author_name << "\n"
    << "Publication year: " << book.publication_year;

    if (!book.tags.empty()) {
        out << "\nTags: ";
        bool comma = false;
        for (const std::string& tag : book.tags) {
            if (comma) {
                out << ", ";
            } else {
                comma = true;
            }
            out << tag;
        }
    }
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
    menu_.AddAction("DeleteAuthor"s, "<name> (optional)"s, "Delete author and all his books"s, std::bind(&View::DeleteAuthor, this, ph::_1));
    menu_.AddAction("DeleteBook"s, "<title> (optional)"s, "Delete book"s, std::bind(&View::DeleteBook, this, ph::_1));
    menu_.AddAction("EditAuthor"s, "<name> (optional)"s, "Edit author's name"s, std::bind(&View::EditAuthor, this, ph::_1));
    menu_.AddAction("EditBook"s, "<title> (optional)"s, "Edit book's info"s, std::bind(&View::EditBook, this, ph::_1));
    menu_.AddAction("ShowBook"s, "<title> (optional)"s, "Show full book info"s, std::bind(&View::ShowBook, this, ph::_1));

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
        boost::algorithm::trim_all(name);

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

        std::optional<std::string> author_id = GetOrSelectAuthor(cmd_input);

        if (author_id) {
            DeleteAuthorBooks(*author_id);
            use_cases_.DeleteAuthor(*author_id);
            use_cases_.Commit();
        }
    } catch (const std::exception&) {
        output_ << "Failed to delete author"sv << std::endl;
    }
    return true;
}

bool View::DeleteBook(std::istream& cmd_input) const {
    try {
        use_cases_.StartTransaction();

        std::optional<std::string> book_id = GetOrSelectBook(cmd_input);
        if (book_id) {
            DeleteTags(*book_id);
            use_cases_.DeleteBook(*book_id);
            use_cases_.Commit();
        }
    } catch (const std::exception&) {
        output_ << "Book not found"sv << std::endl;
    }
    return true;
}

bool View::EditAuthor(std::istream& cmd_input) const {
    try {
        use_cases_.StartTransaction();

        std::optional<std::string> author_id = GetOrSelectAuthor(cmd_input);

        if (author_id) {
            output_ << "Enter new name:"sv << std::endl;
            std::string new_author_name;
            if (std::getline(input_, new_author_name) || !new_author_name.empty()) {
                use_cases_.EditAuthor(*author_id, new_author_name);
                use_cases_.Commit();
            }
        }
    } catch (const std::exception&) {
        output_ << "Failed to edit author"sv << std::endl;
    }
    return true;
}

bool View::EditBook(std::istream& cmd_input) const {
    try {
        use_cases_.StartTransaction();

        std::optional<std::string> book_id = GetOrSelectBook(cmd_input);
        if (book_id) {
            detail::AddBookParams new_book_info = GetBookParamsForEdit(*book_id);
            use_cases_.EditBook(*book_id, new_book_info.author_id, new_book_info.title, new_book_info.publication_year);
            DeleteTags(*book_id);
            for (const auto& tag : new_book_info.tags) {
                use_cases_.AddTag(*book_id, tag);
            }

            use_cases_.Commit();
        } else {
            output_ << "Book not found"sv << std::endl;
        }
    } catch (const std::exception&) {
        output_ << "Book not found"sv << std::endl;
    }
    return true;
}

bool View::ShowAuthors() const {
    use_cases_.StartTransaction();
    PrintVector(output_, GetAuthors());
    return true;
}

bool View::ShowBooks() const {
    use_cases_.StartTransaction();
    PrintVector(output_, GetBooks());
    return true;
}

bool View::ShowAuthorBooks() const {
    // TODO: handle error
    try {
        use_cases_.StartTransaction();
        if (auto author_id = SelectAuthor()) {
            PrintVector(output_, GetAuthorBooks(*author_id));
        }
    } catch (const std::exception&) {
        throw std::runtime_error("Failed to Show Books");
    }
    return true;
}

bool View::ShowBook(std::istream& cmd_input) const {
    try {
        use_cases_.StartTransaction();
        std::optional<std::string> book_id = GetOrSelectBook(cmd_input);

        if (book_id) {
            if (auto full_book_info = GetBook(*book_id)) {
                output_ << *full_book_info << std::endl;
            }
        }
    } catch (const std::exception&) {
        throw std::runtime_error("Book not found");
    }
    return true;
}

std::optional<detail::AddBookParams> View::GetBookParams(std::istream& cmd_input) const {
    detail::AddBookParams params;

    cmd_input >> params.publication_year;
    std::getline(cmd_input, params.title);
    boost::algorithm::trim_all(params.title);

    output_ << "Enter author name or empty line to select from list:"sv << std::endl;
    std::string author_name;
    if (std::getline(input_, author_name) && !author_name.empty()) {
        boost::algorithm::trim_all(author_name);
        auto author = use_cases_.GetAuthorByName(author_name);
        if (!author) {
            output_ << "No author found. Do you want to add "sv << author_name << " (y/n)?"sv << std::endl;
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
    params.tags = GetBookTags();
    return params;
}

detail::AddBookParams View::GetBookParamsForEdit(const std::string& book_id) const {
    detail::AddBookParams new_book_info;

    auto book = use_cases_.GetBookById(book_id);
    if (!book) {
        throw std::runtime_error("empty book");
    }

    new_book_info.author_id = book->GetAuthorId().ToString();

    output_ << "Enter new title or empty line to use the current one ("sv << book->GetTitle() << "):"sv << std::endl;
    std::string str;
    if (std::getline(input_, str) && !str.empty()) {
        new_book_info.title = str;
    } else {
        new_book_info.title = book->GetTitle();
    }

    output_ << "Enter publication year or empty line to use the current one ("sv << book->GetPublicationYear() << "):"sv << std::endl;
    if (std::getline(input_, str) && !str.empty()) {
        try {
            new_book_info.publication_year = std::stoi(str);
        } catch (std::exception const&) {
            throw std::runtime_error("Invalid publication year");
        }
    } else {
        new_book_info.publication_year = book->GetPublicationYear();
    }

    output_ << "Enter tags (current tags: "sv;
    bool comma = false;
    for (const auto& tag : use_cases_.GetBookTags(book_id)) {
        if (comma) {
            output_ << ", "sv;
        } else {
            comma = true;
        }
        output_ << tag;
    }
    output_ << "):"sv << std::endl;

    new_book_info.tags = GetBookTags();

    return new_book_info;
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

std::optional<std::string> View::SelectBook() const {
    auto books = GetBooks();
    PrintVector(output_, books);
    output_ << "Enter the book # or empty line to cancel" << std::endl;

    std::string str;
    if (!std::getline(input_, str) || str.empty()) {
        return std::nullopt;
    }

    int book_idx;
    try {
        book_idx = std::stoi(str);
    } catch (std::exception const&) {
        throw std::runtime_error("Invalid book num");
    }

    --book_idx;
    if (book_idx < 0 or book_idx >= books.size()) {
        throw std::runtime_error("Invalid book num");
    }

    return books[book_idx].id;
}

std::optional<std::string> View::GetOrSelectAuthor(std::istream& cmd_input) const {
    std::string author_name;
    std::getline(cmd_input, author_name);
    boost::algorithm::trim_all(author_name);

    std::optional<std::string> author_id;

    if (author_name.empty()) {
        author_id = SelectAuthor();
    } else {
        auto author = use_cases_.GetAuthorByName(author_name);
        if (!author) {
            throw std::runtime_error("Unknown author");
        }
        author_id = author->GetId().ToString();
    }
    return author_id;
}

std::optional<std::string> View::GetOrSelectBook(std::istream& cmd_input) const {
    std::string book_title;
    std::getline(cmd_input, book_title);
    boost::algorithm::trim_all(book_title);

    std::optional<std::string> book_id;

    if (book_title.empty()) {
        book_id = SelectBook();
    } else {
        auto books = GetBooksByTitle(book_title);
        if (books.empty()) {
            throw std::runtime_error("Unknown book");
        } else if (books.size() == 1) {
            book_id = books.back().id;
        } else {
            PrintVector(output_, books);

            //TO DO: make func:
            std::string str;
            if (!std::getline(input_, str) || str.empty()) {
                return std::nullopt;
            }

            int book_idx;
            try {
                book_idx = std::stoi(str);
            } catch (std::exception const&) {
                throw std::runtime_error("Invalid book num");
            }

            --book_idx;
            if (book_idx < 0 or book_idx >= books.size()) {
                throw std::runtime_error("Invalid book num");
            }

            book_id = books[book_idx].id;
        }
    }
    return book_id;
}

std::vector<detail::AuthorInfo> View::GetAuthors() const {
    std::vector<detail::AuthorInfo> dst_autors;

    for (const auto& author : use_cases_.GetAuthors()) {
        dst_autors.push_back({author.GetId().ToString(), author.GetName()});
    }
    return dst_autors;
}

std::vector<detail::BookInfoWithAuthor> View::GetBooks() const {
    std::vector<detail::BookInfoWithAuthor> books;

    for (const auto& book : use_cases_.GetAllBooks()) {
        auto author = use_cases_.GetAuthorById(book.GetAuthorId().ToString());
        if (!author) {
            throw std::runtime_error("No author in database");
        }
        books.push_back({book.GetBookId().ToString(), book.GetTitle(), book.GetPublicationYear(), author->GetName()});
    }

    return books;
}

std::optional<detail::FullBookInfo> View::GetBook(const std::string& book_id) const {
    auto book = use_cases_.GetBookById(book_id);
    if (!book) {
        return std::nullopt;
    }
    auto author = use_cases_.GetAuthorById(book->GetAuthorId().ToString());
    if (!author) {
        throw std::runtime_error("Unknown author");
    }
    auto tags = use_cases_.GetBookTags(book->GetBookId().ToString());
    return detail::FullBookInfo{book->GetTitle(), book->GetPublicationYear(), author->GetName(), tags};
}

std::vector<detail::BookInfoWithAuthor> View::GetBooksByTitle(const std::string& title) const {
    std::vector<detail::BookInfoWithAuthor> books;
    for (const auto& book : use_cases_.GetBooksByTitle(title)) {
        auto author = use_cases_.GetAuthorById(book.GetAuthorId().ToString());
        books.push_back(detail::BookInfoWithAuthor{.id = book.GetBookId().ToString(), .title = book.GetTitle(),
            .publication_year = book.GetPublicationYear(), .author_name = author->GetName()});
    }
    return books;
}

std::vector<detail::BookInfo> View::GetAuthorBooks(const std::string& author_id) const {
    std::vector<detail::BookInfo> books;

    for (const auto& book : use_cases_.GetAuthorBooks(author_id)) {
        books.push_back({book.GetBookId().ToString(), book.GetTitle(), book.GetPublicationYear()});
    }

    return books;
}

std::vector<std::string> View::GetBookTags() const {
    std::vector<std::string> parsed_tags;
    std::string tags;
    std::getline(input_, tags);

    size_t prev_pos = 0;
    size_t cur_pos = 0;
    while (cur_pos != std::string::npos) {
        cur_pos = tags.find_first_of(',', prev_pos);
        std::string tag = tags.substr(prev_pos, cur_pos - prev_pos);
        boost::algorithm::trim_all(tag);
        
        if (!tag.empty() && std::find(parsed_tags.begin(), parsed_tags.end(), tag) == parsed_tags.end()) {
            parsed_tags.push_back(tag);
        }

        prev_pos = cur_pos + 1;
    }
    std::sort(parsed_tags.begin(), parsed_tags.end());
    return parsed_tags;
}

void View::DeleteTags(const std::string& book_id) const {
    use_cases_.DeleteBookTags(book_id);
}

void View::DeleteAuthorBooks(const std::string& author_id) const {
    auto books = GetAuthorBooks(author_id);

    for (const auto& book_info : books) {
        DeleteTags(book_info.id);
        use_cases_.DeleteBook(book_info.id);
    }
}

}  // namespace ui
