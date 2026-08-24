#pragma once

#include <optional>
#include <string>
#include <vector>

#include "../domain/author.h"
#include "../domain/book.h"

namespace app {

class UseCases {
public:
    virtual domain::AuthorId AddAuthor(const std::string& name) = 0;
    virtual void DeleteAuthor(const std::string& name) = 0;
    virtual void EditAuthor(const std::string& name, const std::string& new_name) = 0;
    virtual std::vector<domain::Author> GetAuthors() = 0;
    virtual void AddBook(const domain::AuthorId& author_id, const std::string& title,
                         int publication_year, const std::vector<std::string>& tags) = 0;
    virtual std::vector<domain::Book> GetBooks() = 0;
    virtual std::vector<domain::Book> GetBooksByTitle(const std::string& title) = 0;
    virtual std::optional<domain::Book> GetBookById(const domain::BookId& id) = 0;
    virtual void DeleteBook(const domain::BookId& id) = 0;
    virtual void EditBook(const domain::Book& book) = 0;
    virtual std::vector<domain::Book> GetAuthorBooks(const domain::AuthorId& author_id) = 0;

protected:
    ~UseCases() = default;
};

}  // namespace app
