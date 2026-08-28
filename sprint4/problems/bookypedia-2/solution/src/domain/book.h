#pragma once
#include <algorithm>
#include <cctype>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "../util/tagged_uuid.h"
#include "author.h"

namespace domain {

namespace detail {
struct BookTag {};
}  // namespace detail

using BookId = util::TaggedUUID<detail::BookTag>;

class Book {
public:
    Book(BookId id, AuthorId author_id, std::string title, int publication_year,
         std::vector<std::string> tags = {}, std::string author_name = {})
        : id_(std::move(id))
        , author_id_(std::move(author_id))
        , title_(std::move(title))
        , publication_year_(publication_year)
        , tags_(std::move(tags))
        , author_name_(std::move(author_name)) {
    }

    const BookId& GetId() const noexcept {
        return id_;
    }

    const AuthorId& GetAuthorId() const noexcept {
        return author_id_;
    }

    const std::string& GetTitle() const noexcept {
        return title_;
    }

    int GetPublicationYear() const noexcept {
        return publication_year_;
    }

    const std::vector<std::string>& GetTags() const noexcept {
        return tags_;
    }

    const std::string& GetAuthorName() const noexcept {
        return author_name_;
    }

private:
    BookId id_;
    AuthorId author_id_;
    std::string title_;
    int publication_year_;
    std::vector<std::string> tags_;
    std::string author_name_;
};

class BookRepository {
public:
    virtual void Save(const Book& book) = 0;
    virtual std::vector<Book> GetAuthorBooks(const AuthorId& author_id) = 0;
    virtual std::vector<Book> GetBooks() = 0;
    virtual std::vector<Book> GetBooksByTitle(const std::string& title) = 0;
    virtual std::optional<Book> GetBookById(const BookId& id) = 0;
    virtual void DeleteBook(const BookId& id) = 0;
    virtual void EditBook(const Book& book) = 0;

protected:
    ~BookRepository() = default;
};

inline std::vector<std::string> NormalizeTags(const std::string& raw_tags) {
    std::vector<std::string> result;
    std::istringstream stream(raw_tags);
    std::string token;
    while (std::getline(stream, token, ',')) {
        const auto begin = token.find_first_not_of(" \t\r\n");
        if (begin == std::string::npos) {
            continue;
        }
        const auto end = token.find_last_not_of(" \t\r\n");
        const std::string trimmed = token.substr(begin, end - begin + 1);

        std::string collapsed;
        bool prev_space = false;
        for (char c : trimmed) {
            if (std::isspace(static_cast<unsigned char>(c))) {
                if (!prev_space) {
                    collapsed += ' ';
                }
                prev_space = true;
            } else {
                collapsed += c;
                prev_space = false;
            }
        }

        if (!collapsed.empty()
            && std::find(result.begin(), result.end(), collapsed) == result.end()) {
            result.push_back(collapsed);
        }
    }
    std::sort(result.begin(), result.end());
    return result;
}

}  // namespace domain
