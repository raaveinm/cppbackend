#include "view.h"

#include <algorithm>
#include <boost/algorithm/string/trim.hpp>
#include <cassert>
#include <iostream>

#include "../app/use_cases.h"
#include "../domain/author.h"
#include "../domain/book.h"
#include "../menu/menu.h"

using namespace std::literals;
namespace ph = std::placeholders;
// ReSharper disable CppUseFamiliarTemplateSyntaxForGenericLambdas
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

std::ostream& operator<<(std::ostream& out, const BookListInfo& book) {
    out << book.title << " by " << book.author_name << ", " << book.publication_year;
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
    menu_.AddAction(  //
        "AddAuthor"s, "name"s, "Adds author"s, [this](auto && PH1) { return AddAuthor(std::forward<decltype(PH1)>(PH1)); }

        // либо
        // [this](auto& cmd_input) { return AddAuthor(cmd_input); }
    );
    menu_.AddAction("DeleteAuthor"s, {}, "Delete author"s, [this](auto && PH1) { return DeleteAuthor(std::forward<decltype(PH1)>(PH1)); });
    menu_.AddAction("EditAuthor"s, {}, "Edit author"s, [this](auto && PH1) { return EditAuthor(std::forward<decltype(PH1)>(PH1)); });

    menu_.AddAction("AddBook"s, "<pub year> <title>"s, "Adds book"s,
                    [this](auto && PH1) { return AddBook(std::forward<decltype(PH1)>(PH1)); });
    menu_.AddAction("ShowBook"s, "title"s, "Show book"s,
                    [this](auto && PH1) { return ShowBook(std::forward<decltype(PH1)>(PH1)); });
    menu_.AddAction("DeleteBook"s, "title"s, "Delete book"s,
                    [this](auto && PH1) { return DeleteBook(std::forward<decltype(PH1)>(PH1)); });
    menu_.AddAction("EditBook"s, "title"s, "Edit book"s,
                    [this](auto && PH1) { return EditBook(std::forward<decltype(PH1)>(PH1)); });
    menu_.AddAction("ShowAuthors"s, {}, "Show authors"s, std::bind(&View::ShowAuthors, this));
    menu_.AddAction("ShowBooks"s, {}, "Show books"s, std::bind(&View::ShowBooks, this));
    menu_.AddAction("ShowAuthorBooks"s, {}, "Show author books"s,
                    std::bind(&View::ShowAuthorBooks, this));
}

bool View::AddAuthor(std::istream& cmd_input) const {
    try {
        std::string name;
        std::getline(cmd_input, name);
        boost::algorithm::trim(name);
        if (name.empty()) {
            throw std::runtime_error("Author name is empty");
        }
        use_cases_.AddAuthor(std::move(name));
    } catch (const std::exception&) {
        output_ << "Failed to add author"sv << std::endl;
    }
    return true;
}

bool View::DeleteAuthor(std::istream& cmd_input) const {
    try {
        std::string name;
        std::getline(cmd_input, name);
        boost::algorithm::trim(name);

        if (name.empty()) {
            auto author = SelectAuthorInfo(false);
            if (!author) {
                return true;
            }
            name = author->name;
        }

        use_cases_.DeleteAuthor(name);
    } catch (const std::exception&) {
        output_ << "Failed to delete author"sv << std::endl;
    }
    return true;
}

bool View::EditAuthor(std::istream& cmd_input) const {
    try {
        std::string name;
        std::getline(cmd_input, name);
        boost::algorithm::trim(name);

        if (name.empty()) {
            auto author = SelectAuthorInfo(true);
            if (!author) {
                return true;
            }
            name = author->name;
        }

        output_ << "Enter new name:" << std::endl;
        std::string new_name;
        std::getline(input_, new_name);
        boost::algorithm::trim(new_name);
        if (new_name.empty()) {
            throw std::runtime_error("New author name is empty");
        }

        use_cases_.EditAuthor(name, new_name);
    } catch (const std::exception&) {
        output_ << "Failed to edit author"sv << std::endl;
    }
    return true;
}

bool View::AddBook(std::istream& cmd_input) const {
    try {
        if (const auto params = GetBookParams(cmd_input)) {
            use_cases_.AddBook(domain::AuthorId::FromString(params->author_id), params->title,
                                params->publication_year, params->tags);
        }
    } catch (const std::exception&) {
        output_ << "Failed to add book"sv << std::endl;
    }
    return true;
}

bool View::ShowBook(std::istream& cmd_input) const {
    try {
        std::string title;
        std::getline(cmd_input, title);
        boost::algorithm::trim(title);

        auto candidates = title.empty() ? GetBookList() : GetBookListByTitle(title);
        if (candidates.empty()) {
            return true;
        }

        auto selected = SelectBookFromCandidates(candidates);
        if (!selected) {
            return true;
        }

        auto book = use_cases_.GetBookById(domain::BookId::FromString(selected->id));
        if (!book) {
            return true;
        }

        output_ << "Title: " << book->GetTitle() << std::endl;
        output_ << "Author: " << book->GetAuthorName() << std::endl;
        output_ << "Publication year: " << book->GetPublicationYear() << std::endl;
        if (!book->GetTags().empty()) {
            output_ << "Tags: ";
            bool first = true;
            for (const auto& tag : book->GetTags()) {
                if (!first) {
                    output_ << ", ";
                }
                output_ << tag;
                first = false;
            }
            output_ << std::endl;
        }
    } catch (const std::exception&) {
        // ShowBook has no defined failure message; matches with nothing displayed.
    }
    return true;
}

bool View::DeleteBook(std::istream& cmd_input) const {
    try {
        std::string title;
        std::getline(cmd_input, title);
        boost::algorithm::trim(title);

        auto candidates = title.empty() ? GetBookList() : GetBookListByTitle(title);
        if (candidates.empty()) {
            throw std::runtime_error("Book not found");
        }

        auto selected = SelectBookFromCandidates(candidates);
        if (!selected) {
            return true;
        }

        use_cases_.DeleteBook(domain::BookId::FromString(selected->id));
    } catch (const std::exception& e) {
        output_ << e.what() << std::endl;
    }
    return true;
}

bool View::EditBook(std::istream& cmd_input) const {
    try {
        std::string title;
        std::getline(cmd_input, title);
        boost::algorithm::trim(title);

        auto candidates = title.empty() ? GetBookList() : GetBookListByTitle(title);
        if (candidates.empty()) {
            throw std::runtime_error("Book not found");
        }

        auto selected = SelectBookFromCandidates(candidates);
        if (!selected) {
            throw std::runtime_error("Book not found");
        }

        auto book = use_cases_.GetBookById(domain::BookId::FromString(selected->id));
        if (!book) {
            throw std::runtime_error("Book not found");
        }

        output_ << "Enter new title or empty line to use the current one (" << book->GetTitle()
                << "):" << std::endl;
        std::string new_title;
        std::getline(input_, new_title);
        boost::algorithm::trim(new_title);
        if (new_title.empty()) {
            new_title = book->GetTitle();
        }

        output_ << "Enter publication year or empty line to use the current one ("
                << book->GetPublicationYear() << "):" << std::endl;
        std::string year_line;
        std::getline(input_, year_line);
        boost::algorithm::trim(year_line);
        int new_year = book->GetPublicationYear();
        if (!year_line.empty()) {
            new_year = std::stoi(year_line);
        }

        std::string current_tags;
        bool first = true;
        for (const auto& tag : book->GetTags()) {
            if (!first) {
                current_tags += ", ";
            }
            current_tags += tag;
            first = false;
        }
        output_ << "Enter tags (current tags: " << current_tags << "):" << std::endl;
        std::string tags_line;
        std::getline(input_, tags_line);
        auto new_tags = domain::NormalizeTags(tags_line);

        use_cases_.EditBook(domain::Book{book->GetId(), book->GetAuthorId(), new_title, new_year,
                                          new_tags});
    } catch (const std::exception&) {
        output_ << "Book not found"sv << std::endl;
    }
    return true;
}

bool View::ShowAuthors() const {
    PrintVector(output_, GetAuthors());
    return true;
}

bool View::ShowBooks() const {
    PrintVector(output_, GetBookList());
    return true;
}

bool View::ShowAuthorBooks() const {
    try {
        if (const auto author_id = SelectAuthor()) {
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

    output_ << "Enter author name or empty line to select from list:" << std::endl;
    std::string author_name;
    std::getline(input_, author_name);
    boost::algorithm::trim(author_name);

    if (author_name.empty()) {
        auto author_id = SelectAuthor();
        if (!author_id) {
            return std::nullopt;
        }
        params.author_id = *author_id;
    } else {
        auto authors = GetAuthors();
        auto it = std::find_if(authors.begin(), authors.end(), [&](const detail::AuthorInfo& a) {
            return a.name == author_name;
        });
        if (it != authors.end()) {
            params.author_id = it->id;
        } else {
            output_ << "No author found. Do you want to add " << author_name << " (y/n)?"
                    << std::endl;
            std::string answer;
            std::getline(input_, answer);
            if (answer != "y" && answer != "Y") {
                throw std::runtime_error("Author addition declined");
            }
            params.author_id = use_cases_.AddAuthor(author_name).ToString();
        }
    }

    output_ << "Enter tags (comma separated):" << std::endl;
    std::string tags_line;
    std::getline(input_, tags_line);
    params.tags = domain::NormalizeTags(tags_line);

    return params;
}

std::optional<detail::AuthorInfo> View::SelectAuthorInfo(bool print_header) const {
    if (print_header) {
        output_ << "Select author:" << std::endl;
    }
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
    if (author_idx < 0 or author_idx >= static_cast<int>(authors.size())) {
        throw std::runtime_error("Invalid author num");
    }

    return authors[author_idx];
}

std::optional<std::string> View::SelectAuthor() const {
    auto author = SelectAuthorInfo(true);
    if (!author) {
        return std::nullopt;
    }
    return author->id;
}

std::optional<detail::BookListInfo> View::SelectBookFromCandidates(
    const std::vector<detail::BookListInfo>& candidates) const {
    if (candidates.size() == 1) {
        return candidates.front();
    }

    PrintVector(output_, candidates);
    output_ << "Enter the book # or empty line to cancel:" << std::endl;

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
    if (book_idx < 0 or book_idx >= static_cast<int>(candidates.size())) {
        throw std::runtime_error("Invalid book num");
    }

    return candidates[book_idx];
}

std::vector<detail::AuthorInfo> View::GetAuthors() const {
    std::vector<detail::AuthorInfo> dst_autors;
    for (const auto& author : use_cases_.GetAuthors()) {
        dst_autors.push_back({author.GetId().ToString(), author.GetName()});
    }
    return dst_autors;
}

std::vector<detail::BookListInfo> View::GetBookList() const {
    std::vector<detail::BookListInfo> books;
    for (const auto& book : use_cases_.GetBooks()) {
        books.push_back({book.GetId().ToString(), book.GetTitle(), book.GetAuthorName(),
                          book.GetPublicationYear()});
    }
    return books;
}

std::vector<detail::BookListInfo> View::GetBookListByTitle(const std::string& title) const {
    std::vector<detail::BookListInfo> books;
    for (const auto& book : use_cases_.GetBooksByTitle(title)) {
        books.push_back({book.GetId().ToString(), book.GetTitle(), book.GetAuthorName(),
                          book.GetPublicationYear()});
    }
    return books;
}

std::vector<detail::BookInfo> View::GetAuthorBooks(const std::string& author_id) const {
    std::vector<detail::BookInfo> books;
    for (const auto& book : use_cases_.GetAuthorBooks(domain::AuthorId::FromString(author_id))) {
        books.push_back({book.GetTitle(), book.GetPublicationYear()});
    }
    return books;
}

}  // namespace ui
