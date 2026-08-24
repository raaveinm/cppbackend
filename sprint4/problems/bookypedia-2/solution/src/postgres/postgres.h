#pragma once
#include <pqxx/connection>
#include <pqxx/transaction>

#include "../domain/author.h"
#include "../domain/book.h"

namespace postgres {

class AuthorRepositoryImpl : public domain::AuthorRepository {
public:
    explicit AuthorRepositoryImpl(pqxx::connection& connection)
        : connection_{connection} {
    }

    void Save(const domain::Author& author) override;
    std::vector<domain::Author> GetAuthors() override;
    void DeleteAuthor(const std::string& name) override;
    void EditAuthor(const std::string& name, const std::string& new_name) override;

private:
    pqxx::connection& connection_;
};

class BookRepositoryImpl : public domain::BookRepository {
public:
    explicit BookRepositoryImpl(pqxx::connection& connection)
        : connection_{connection} {
    }

    void Save(const domain::Book& book) override;
    std::vector<domain::Book> GetAuthorBooks(const domain::AuthorId& author_id) override;
    std::vector<domain::Book> GetBooks() override;
    std::vector<domain::Book> GetBooksByTitle(const std::string& title) override;
    std::optional<domain::Book> GetBookById(const domain::BookId& id) override;
    void DeleteBook(const domain::BookId& id) override;
    void EditBook(const domain::Book& book) override;

private:
    pqxx::connection& connection_;

    void SaveTags(pqxx::work& work, const domain::BookId& book_id,
                  const std::vector<std::string>& tags);
};

class Database {
public:
    explicit Database(pqxx::connection connection);

    AuthorRepositoryImpl& GetAuthors() & {
        return authors_;
    }

    BookRepositoryImpl& GetBooks() & {
        return books_;
    }

private:
    pqxx::connection connection_;
    AuthorRepositoryImpl authors_{connection_};
    BookRepositoryImpl books_{connection_};
};

}  // namespace postgres
