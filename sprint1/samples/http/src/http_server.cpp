//
// Created by raaveinm on 6/20/26.
//

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <iostream>
#include <optional>
#include <thread>

namespace net = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;

using boost::asio::ip::tcp;
using StringRequest = http::request<http::string_body>;
using StringResponse = http::response<http::string_body>;

struct ContentType {
    ContentType() = delete;
    constexpr static std::string_view TEXT_HTML = "text/html";
    constexpr static std::string_view TEXT_PLAIN = "text/plain";
};

///////////////////////////////////////////////
// Read User Request
///////////////////////////////////////////////
std::optional<StringRequest> ReadRequest(tcp::socket& socket, beast::flat_buffer& buffer) {
    beast::error_code ec;
    StringRequest req;
    http::read(socket, buffer, req, ec);

    if (ec == http::error::end_of_stream)
        return std::nullopt;
    if (ec)
        throw std::runtime_error("Failed to read request: " + ec.message());

    return req;
}

///////////////////////////////////////////////
// Make Response
///////////////////////////////////////////////
StringResponse MakeStringResponse(
    const http::status status,
    const std::string_view body,
    const unsigned short version,
    const bool keep_alive,
    const std::string_view content_type = ContentType::TEXT_HTML
) {
    StringResponse resp(status, version);                        // declaring response
    resp.set(http::field::content_type, content_type);      // ok
    resp.body() = body;                                          // body
    resp.content_length(resp.body().size());             // response size
    resp.keep_alive(keep_alive);                                 // keeping connection
    return resp;
}

///////////////////////////////////////////////
// Display Request
///////////////////////////////////////////////
void DumpRequest(const StringRequest& req) {
    std::cout << req.method_string() << ' ' << req.target() << std::endl;
    // print request headers
    for (const auto& header : req) {
        std::cout << "  " << header.name_string() << ": " << header.value() << std::endl;
    }
}


///////////////////////////////////////////////
// Handle Request
///////////////////////////////////////////////
StringResponse HandleRequest(StringRequest&& req) {
    const auto text_response = [&req](const http::status status, const std::string_view text) {
        return MakeStringResponse(status, text, req.version(), req.keep_alive());
    };

    // parse request and make answer
    return text_response( // dummy
        http::status::ok, // its always "ok" & always Rick Roll
        "<a href=\"https://www.youtube.com/watch?v=dQw4w9WgXcQ\">Bonjour</strong>");
}

///////////////////////////////////////////////
// Handle Connection
///////////////////////////////////////////////
void HandleConnection_Simple(tcp::socket& socket) {
    try {
        // session buffer
        beast::flat_buffer buffer;

        // parse requests whey they are coming
        while (auto req = ReadRequest(socket, buffer)) {
            DumpRequest(*req);

            // response
            StringResponse resp(http::status::ok, req->version()); // ok
            resp.set(http::field::content_type, "text/html"); // header
            resp.body() = "<a href=\"https://www.youtube.com/"          // body
                          "watch?v=dQw4w9WgXcQ\">Bonjour</strong>";     // body, but on second line
            resp.content_length(resp.body().size());            // response size
            resp.keep_alive(req->keep_alive());                    // keeping connection
            http::write(socket, resp);                               // send response
            if (resp.need_eof()) break;                                 // close connection by request
        }

    } catch (std::exception &e) { std::cerr << e.what() << std::endl; }

    // close connection
    beast::error_code error;
    socket.shutdown(tcp::socket::shutdown_send, error);
}

// alt
template <typename RequestHandler>
void HandleConnection(tcp::socket& socket, RequestHandler&& handle_request) {
    try {
        // session buffer
        beast::flat_buffer buffer;

        // parse requests whey they are coming
        while (auto request = ReadRequest(socket, buffer)) {
            DumpRequest(*request);
            StringResponse response = handle_request(*std::move(request));
            http::write(socket, response);
            if (response.need_eof()) {
                break;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    beast::error_code ec;
    // close connection
    socket.shutdown(tcp::socket::shutdown_send, ec);
}

///////////////////////////////////////////////
// Main
///////////////////////////////////////////////
int main() {
    // initialization
    net::io_context io_context;
    const auto ADDRESS = net::ip::make_address("0.0.0.0");
    constexpr unsigned short PORT = 8080;

    // connection
    tcp::acceptor acceptor(io_context, {ADDRESS, PORT});

    // handling connections
    while (true) {
        tcp::socket socket(io_context);
        acceptor.accept(socket);

        // launch new thread for each connection
        std::thread t(
            [](tcp::socket _socket) {
                HandleConnection(_socket, HandleRequest);
            },
            std::move(socket));
        t.detach();
    }
}