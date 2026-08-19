//
// Created by raaveinm on 8/19/26.
//
#include <iostream>
#include <pqxx/pqxx>
#include <boost/json.hpp>

using pqxx::operator""_zv;
namespace json = boost::json;

static void CreateTable(pqxx::connection& connection) {
    pqxx::work work(connection);
    const std::string table_creation_query = R"(
            CREATE TABLE IF NOT EXISTS books (
                id SERIAL PRIMARY KEY NOT NULL,
                title VARCHAR(100) NOT NULL,
                author VARCHAR(100) NOT NULL,
                year INTEGER NOT NULL,
                ISBN CHAR(13) UNIQUE
            );
        )";
    work.exec(table_creation_query);
    work.commit();
}

static void AddBook(pqxx::connection& connection,const boost::json::object& book) {
    std::string title = book.at("title").as_string().c_str();
    std::string author = book.at("author").as_string().c_str();
    int year = static_cast<int>(book.at("year").as_int64());

    std::optional<std::string> isbn;
    if (!book.at("ISBN").is_null()) {
        isbn = book.at("ISBN").as_string().c_str();
    }

    try {
        pqxx::work tx{connection};
        tx.exec_params(
            "INSERT INTO books (title, author, year, ISBN) VALUES ($1, $2, $3, $4);",
            title, author, year, isbn
        );
        tx.commit();
        std::cout << json::serialize(json::value{{"result", true}}) << "\n";
    } catch (const pqxx::sql_error&) {
        std::cout << json::serialize(json::value{{"result", false}}) << "\n";
    }
}

static void HandleBooks(pqxx::connection& conn) {
    pqxx::read_transaction tx{conn};
    const auto rows = tx.exec(R"(
        SELECT id, title, author, year, ISBN
        FROM books
        ORDER BY year DESC, title ASC, author ASC, ISBN ASC;
    )");

    json::array result_arr;
    for (const auto& row : rows) {
        json::object book;
        book["id"] = row["id"].as<int>();
        book["title"] = row["title"].as<std::string>();
        book["author"] = row["author"].as<std::string>();
        book["year"] = row["year"].as<int>();

        if (row["ISBN"].is_null()) {
            book["ISBN"] = nullptr;
        } else {
            book["ISBN"] = row["ISBN"].as<std::string>();
        }
        result_arr.push_back(std::move(book));
    }
    std::cout << json::serialize(result_arr) << "\n";
}

int main(const int argc, const char *argv[]) {

    if (argc != 2) {
        std::cout << "invalid usage\n";
        return 1;
    }

    try {
        pqxx::connection conn{argv[1]};
        CreateTable(conn);

        std::string line;
        while (std::getline(std::cin, line)) {
            if (line.empty()) continue;

            auto parsed = json::parse(line).as_object();
            std::string action = parsed.at("action").as_string().c_str();

            if (action == "exit") {
                break;
            } else if (action == "add_book") {
                AddBook(conn, parsed.at("payload").as_object());
            } else if (action == "all_books") {
                HandleBooks(conn);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 2;
    }
    return 0;
}