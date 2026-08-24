#include "use_cases_impl.h"

#include "../domain/author.h"
#include "../domain/book.h"

namespace app {
using namespace domain;

AuthorId UseCasesImpl::AddAuthor(const std::string& name) {
    auto id = AuthorId::New();
    authors_.Save({id, name});
    return id;
}

std::vector<Author> UseCasesImpl::GetAuthors() {
    return authors_.GetAuthors();
}

void UseCasesImpl::AddBook(const AuthorId& author_id, const std::string& title,
                            int publication_year, const std::vector<std::string>& tags) {
    books_.Save({BookId::New(), author_id, title, publication_year, tags});
}

std::vector<Book> UseCasesImpl::GetBooks() {
    return books_.GetBooks();
}

std::vector<Book> UseCasesImpl::GetBooksByTitle(const std::string& title) {
    return books_.GetBooksByTitle(title);
}

std::optional<Book> UseCasesImpl::GetBookById(const BookId& id) {
    return books_.GetBookById(id);
}

void UseCasesImpl::DeleteBook(const BookId& id) {
    books_.DeleteBook(id);
}

void UseCasesImpl::EditBook(const Book& book) {
    books_.EditBook(book);
}

std::vector<Book> UseCasesImpl::GetAuthorBooks(const AuthorId& author_id) {
    return books_.GetAuthorBooks(author_id);
}

void UseCasesImpl::DeleteAuthor(const std::string &name) {
    authors_.DeleteAuthor(name);
}

void UseCasesImpl::EditAuthor(const std::string& name, const std::string& new_name) {
    authors_.EditAuthor(name, new_name);
}
}  // namespace app
