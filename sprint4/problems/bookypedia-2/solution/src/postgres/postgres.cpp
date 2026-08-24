#include "postgres.h"

#include <pqxx/result>
#include <pqxx/zview.hxx>

namespace postgres {

using namespace std::literals;
using pqxx::operator"" _zv;

void AuthorRepositoryImpl::Save(const domain::Author& author) {
    // Пока каждое обращение к репозиторию выполняется внутри отдельной транзакции
    // В будущих уроках вы узнаете про паттерн Unit of Work, при помощи которого сможете несколько
    // запросов выполнить в рамках одной транзакции.
    // Вы также может самостоятельно почитать информацию про этот паттерн и применить его здесь.
    pqxx::work work{connection_};
    work.exec_params(
        R"(
INSERT INTO authors (id, name) VALUES ($1, $2)
ON CONFLICT (id) DO UPDATE SET name=$2;
)"_zv,
        author.GetId().ToString(), author.GetName());
    work.commit();
}

void AuthorRepositoryImpl::DeleteAuthor(const std::string& name) {
    pqxx::work work{connection_};
    auto id_row = work.exec_params(R"(SELECT id FROM authors WHERE name=$1;)"_zv, name);
    if (id_row.empty()) {
        throw std::runtime_error("Author not found");
    }
    auto author_id = id_row[0][0].as<std::string>();
    // book_tags rows are removed automatically via ON DELETE CASCADE.
    work.exec_params(R"(DELETE FROM books WHERE author_id=$1;)"_zv, author_id);
    work.exec_params(R"(DELETE FROM authors WHERE id=$1;)"_zv, author_id);
    work.commit();
}

void AuthorRepositoryImpl::EditAuthor(const std::string& name, const std::string& new_name) {
    pqxx::work work{connection_};
    auto result = work.exec_params(R"(UPDATE authors SET name=$2 WHERE name=$1;)"_zv, name, new_name);
    if (result.affected_rows() != 1) {
        throw std::runtime_error("Author not found");
    }
    work.commit();
}

std::vector<domain::Author> AuthorRepositoryImpl::GetAuthors() {
    pqxx::read_transaction read{connection_};
    std::vector<domain::Author> authors;
    for (const auto& row : read.exec(R"(SELECT id, name FROM authors ORDER BY name;)"_zv)) {
        authors.emplace_back(domain::AuthorId::FromString(row["id"].as<std::string>()),
                              row["name"].as<std::string>());
    }
    return authors;
}

void BookRepositoryImpl::SaveTags(pqxx::work& work, const domain::BookId& book_id,
                                   const std::vector<std::string>& tags) {
    for (const auto& tag : tags) {
        work.exec_params(
            R"(INSERT INTO book_tags (book_id, tag) VALUES ($1, $2);)"_zv,
            book_id.ToString(), tag);
    }
}

void BookRepositoryImpl::Save(const domain::Book& book) {
    pqxx::work work{connection_};
    work.exec_params(
        R"(
INSERT INTO books (id, author_id, title, publication_year) VALUES ($1, $2, $3, $4);
)"_zv,
        book.GetId().ToString(), book.GetAuthorId().ToString(), book.GetTitle(),
        book.GetPublicationYear());
    SaveTags(work, book.GetId(), book.GetTags());
    work.commit();
}

std::vector<domain::Book> BookRepositoryImpl::GetAuthorBooks(const domain::AuthorId& author_id) {
    pqxx::read_transaction read{connection_};
    std::vector<domain::Book> books;
    for (const auto& row : read.exec_params(
             R"(
SELECT id, author_id, title, publication_year FROM books
WHERE author_id = $1
ORDER BY publication_year, title;
)"_zv,
             author_id.ToString())) {
        books.emplace_back(domain::BookId::FromString(row["id"].as<std::string>()),
                            domain::AuthorId::FromString(row["author_id"].as<std::string>()),
                            row["title"].as<std::string>(),
                            row["publication_year"].as<int>());
    }
    return books;
}

std::vector<domain::Book> BookRepositoryImpl::GetBooks() {
    pqxx::read_transaction read{connection_};
    std::vector<domain::Book> books;
    for (const auto& row : read.exec(
             R"(
SELECT b.id, b.author_id, b.title, b.publication_year, a.name AS author_name
FROM books b JOIN authors a ON b.author_id = a.id
ORDER BY b.title, a.name, b.publication_year;
)"_zv)) {
        books.emplace_back(domain::BookId::FromString(row["id"].as<std::string>()),
                            domain::AuthorId::FromString(row["author_id"].as<std::string>()),
                            row["title"].as<std::string>(), row["publication_year"].as<int>(),
                            std::vector<std::string>{}, row["author_name"].as<std::string>());
    }
    return books;
}

std::vector<domain::Book> BookRepositoryImpl::GetBooksByTitle(const std::string& title) {
    pqxx::read_transaction read{connection_};
    std::vector<domain::Book> books;
    for (const auto& row : read.exec_params(
             R"(
SELECT b.id, b.author_id, b.title, b.publication_year, a.name AS author_name
FROM books b JOIN authors a ON b.author_id = a.id
WHERE b.title = $1
ORDER BY a.name, b.publication_year;
)"_zv,
             title)) {
        books.emplace_back(domain::BookId::FromString(row["id"].as<std::string>()),
                            domain::AuthorId::FromString(row["author_id"].as<std::string>()),
                            row["title"].as<std::string>(), row["publication_year"].as<int>(),
                            std::vector<std::string>{}, row["author_name"].as<std::string>());
    }
    return books;
}

std::optional<domain::Book> BookRepositoryImpl::GetBookById(const domain::BookId& id) {
    pqxx::read_transaction read{connection_};
    auto book_rows = read.exec_params(
        R"(
SELECT b.id, b.author_id, b.title, b.publication_year, a.name AS author_name
FROM books b JOIN authors a ON b.author_id = a.id
WHERE b.id = $1;
)"_zv,
        id.ToString());
    if (book_rows.empty()) {
        return std::nullopt;
    }
    const auto& row = book_rows[0];

    std::vector<std::string> tags;
    for (const auto& tag_row : read.exec_params(
             R"(SELECT tag FROM book_tags WHERE book_id = $1 ORDER BY tag;)"_zv, id.ToString())) {
        tags.push_back(tag_row["tag"].as<std::string>());
    }

    return domain::Book{domain::BookId::FromString(row["id"].as<std::string>()),
                         domain::AuthorId::FromString(row["author_id"].as<std::string>()),
                         row["title"].as<std::string>(), row["publication_year"].as<int>(),
                         std::move(tags), row["author_name"].as<std::string>()};
}

void BookRepositoryImpl::DeleteBook(const domain::BookId& id) {
    pqxx::work work{connection_};
    // book_tags rows are removed automatically via ON DELETE CASCADE.
    auto result = work.exec_params(R"(DELETE FROM books WHERE id=$1;)"_zv, id.ToString());
    if (result.affected_rows() != 1) {
        throw std::runtime_error("Book not found");
    }
    work.commit();
}

void BookRepositoryImpl::EditBook(const domain::Book& book) {
    pqxx::work work{connection_};
    auto result = work.exec_params(
        R"(UPDATE books SET title=$2, publication_year=$3 WHERE id=$1;)"_zv,
        book.GetId().ToString(), book.GetTitle(), book.GetPublicationYear());
    if (result.affected_rows() != 1) {
        throw std::runtime_error("Book not found");
    }
    work.exec_params(R"(DELETE FROM book_tags WHERE book_id=$1;)"_zv, book.GetId().ToString());
    SaveTags(work, book.GetId(), book.GetTags());
    work.commit();
}

    ///////////////////////////////////////////////
    /// Database connection and initialization
    ///////////////////////////////////////////////

Database::Database(pqxx::connection connection)
    : connection_{std::move(connection)} {
    pqxx::work work{connection_};
    // region : authors
    work.exec(R"(
CREATE TABLE IF NOT EXISTS authors (
    id UUID CONSTRAINT author_id_constraint PRIMARY KEY,
    name varchar(100) UNIQUE NOT NULL
);
)"_zv);
    //endregion
    // region : books
    work.exec(R"(
CREATE TABLE IF NOT EXISTS books (
    id UUID CONSTRAINT book_id_constraint PRIMARY KEY,
    author_id UUID NOT NULL CONSTRAINT fk_author REFERENCES authors (id),
    title VARCHAR(100) NOT NULL,
    publication_year INTEGER
);
)"_zv);
    //endregion
    //region : book_tags
    work.exec(R"(
CREATE TABLE IF NOT EXISTS book_tags (
    book_id UUID NOT NULL CONSTRAINT fk_book_tags_book REFERENCES books (id) ON DELETE CASCADE,
    tag VARCHAR(30) NOT NULL,
    CONSTRAINT pk_book_tags PRIMARY KEY (book_id, tag)
);
)");
    //endregion
    // коммитим изменения
    work.commit();
}

}  // namespace postgres
