#pragma once
#include "../domain/author_fwd.h"
#include "../domain/book_fwd.h"
#include "use_cases.h"

namespace app {

class UseCasesImpl : public UseCases {
public:
    virtual ~UseCasesImpl() = default;

    UseCasesImpl(domain::AuthorRepository& authors, domain::BookRepository& books)
        : authors_{authors}
        , books_{books} {
    }

    domain::AuthorId AddAuthor(const std::string& name) override;
    void DeleteAuthor(const std::string& name) override;
    void EditAuthor(const std::string& name, const std::string& new_name) override;
    std::vector<domain::Author> GetAuthors() override;
    void AddBook(const domain::AuthorId& author_id, const std::string& title,
                 int publication_year, const std::vector<std::string>& tags) override;
    std::vector<domain::Book> GetBooks() override;
    std::vector<domain::Book> GetBooksByTitle(const std::string& title) override;
    std::optional<domain::Book> GetBookById(const domain::BookId& id) override;
    void DeleteBook(const domain::BookId& id) override;
    void EditBook(const domain::Book& book) override;
    std::vector<domain::Book> GetAuthorBooks(const domain::AuthorId& author_id) override;

private:
    domain::AuthorRepository& authors_;
    domain::BookRepository& books_;
};

}  // namespace app
