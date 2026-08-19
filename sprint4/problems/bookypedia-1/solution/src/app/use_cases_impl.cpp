#include "use_cases_impl.h"

#include "../domain/author.h"
#include "../domain/book.h"

namespace app {
using namespace domain;

void UseCasesImpl::AddAuthor(const std::string& name) {
    authors_.Save({AuthorId::New(), name});
}

std::vector<Author> UseCasesImpl::GetAuthors() {
    return authors_.GetAuthors();
}

void UseCasesImpl::AddBook(const AuthorId& author_id, const std::string& title,
                            int publication_year) {
    books_.Save({BookId::New(), author_id, title, publication_year});
}

std::vector<Book> UseCasesImpl::GetBooks() {
    return books_.GetBooks();
}

std::vector<Book> UseCasesImpl::GetAuthorBooks(const AuthorId& author_id) {
    return books_.GetAuthorBooks(author_id);
}

}  // namespace app
