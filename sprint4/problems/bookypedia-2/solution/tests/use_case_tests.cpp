#include <catch2/catch_test_macros.hpp>

#include "../src/app/use_cases_impl.h"
#include "../src/domain/author.h"
#include "../src/domain/book.h"

namespace {

struct MockAuthorRepository : domain::AuthorRepository {
    std::vector<domain::Author> saved_authors;

    void Save(const domain::Author& author) override {
        saved_authors.emplace_back(author);
    }

    std::vector<domain::Author> GetAuthors() override {
        return saved_authors;
    }

    void DeleteAuthor(const std::string& name) override {
        std::erase_if(saved_authors,
                       [&](const domain::Author& author) { return author.GetName() == name; });
    }

    void EditAuthor(const std::string& name, const std::string& new_name) override {
        for (auto& author : saved_authors) {
            if (author.GetName() == name) {
                author = domain::Author{author.GetId(), new_name};
                return;
            }
        }
        throw std::runtime_error("Author not found");
    }
};

struct MockBookRepository : domain::BookRepository {
    std::vector<domain::Book> saved_books;

    void Save(const domain::Book& book) override {
        saved_books.emplace_back(book);
    }

    std::vector<domain::Book> GetAuthorBooks(const domain::AuthorId& author_id) override {
        std::vector<domain::Book> result;
        for (const auto& book : saved_books) {
            if (book.GetAuthorId() == author_id) {
                result.push_back(book);
            }
        }
        return result;
    }

    std::vector<domain::Book> GetBooks() override {
        return saved_books;
    }

    std::vector<domain::Book> GetBooksByTitle(const std::string& title) override {
        std::vector<domain::Book> result;
        for (const auto& book : saved_books) {
            if (book.GetTitle() == title) {
                result.push_back(book);
            }
        }
        return result;
    }

    std::optional<domain::Book> GetBookById(const domain::BookId& id) override {
        for (const auto& book : saved_books) {
            if (book.GetId() == id) {
                return book;
            }
        }
        return std::nullopt;
    }

    void DeleteBook(const domain::BookId& id) override {
        std::erase_if(saved_books, [&](const domain::Book& book) { return book.GetId() == id; });
    }

    void EditBook(const domain::Book& book) override {
        for (auto& saved_book : saved_books) {
            if (saved_book.GetId() == book.GetId()) {
                saved_book = book;
                return;
            }
        }
        throw std::runtime_error("Book not found");
    }
};

struct Fixture {
    MockAuthorRepository authors;
    MockBookRepository books;
};

}  // namespace

SCENARIO_METHOD(Fixture, "Book Adding") {
    GIVEN("Use cases") {
        app::UseCasesImpl use_cases{authors, books};

        WHEN("Adding an author") {
            const auto author_name = "Joanne Rowling";
            use_cases.AddAuthor(author_name);

            THEN("author with the specified name is saved to repository") {
                REQUIRE(authors.saved_authors.size() == 1);
                CHECK(authors.saved_authors.at(0).GetName() == author_name);
                CHECK(authors.saved_authors.at(0).GetId() != domain::AuthorId{});
            }
        }
    }
}
