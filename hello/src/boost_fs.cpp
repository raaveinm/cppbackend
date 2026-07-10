//
// Created by raaveinm on 7/8/26.
//


#include <iostream>
#include <boost/beast.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
namespace sys = boost::system;

int main(const int argc, const char *argv[]) {
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <path/to/file>\n";
        return EXIT_FAILURE;
    }

    http::response<http::file_body> response;
    response.version(11);
    response.result(http::status::ok);
    response.insert(http::field::content_type, "text/plain");
    http::file_body::value_type file;
    if (sys::error_code error_code; file.open(argv[1], beast::file_mode::read, error_code), error_code) {
        std::cout << "Failed to read file: " << error_code.message() << "\n";
        return EXIT_FAILURE;
    }
    response.body() = std::move(file);
    response.prepare_payload();
}
