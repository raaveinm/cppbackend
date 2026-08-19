#pragma once

#include <string>
#include <vector>

#include "../domain/author.h"
#include "../domain/book.h"

namespace app {

class UseCases {
public:
    virtual void AddAuthor(const std::string& name) = 0;
    virtual std::vector<domain::Author> GetAuthors() = 0;
    virtual void AddBook(const domain::AuthorId& author_id, const std::string& title,
                          int publication_year) = 0;
    virtual std::vector<domain::Book> GetBooks() = 0;
    virtual std::vector<domain::Book> GetAuthorBooks(const domain::AuthorId& author_id) = 0;

protected:
    ~UseCases() = default;
};

}  // namespace app
