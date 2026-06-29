#ifdef WIN32
#include <sdkddkver.h>
#endif
// boost.beast будет использовать std::string_view вместо boost::string_view
#define BOOST_BEAST_USE_STD_STRING_VIEW

#include <boost/asio/ip/tcp.hpp>
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

    if (req.method() == http::verb::get || req.method() == http::verb::head) {
        const std::string hello_name = "Hello, " + std::string(req.target().substr(1));  // parse name
        const unsigned short hello_size = hello_name.size();

        StringResponse resp(http::status::ok, req.version());              // declaring response
        resp.set(http::field::content_type, ContentType::TEXT_HTML);  // content type
        resp.content_length(hello_size);                                     // response size
        resp.keep_alive(req.keep_alive());                                 // keep connection
        if (req.method() == http::verb::get) resp.body() = hello_name;          // set body

        return resp;
    }

    StringResponse resp(http::status::method_not_allowed, req.version());  // declaring response
    resp.set(http::field::content_type, ContentType::TEXT_HTML);      // content type
    resp.set(http::field::allow, "GET, HEAD");                        // Allowed methods
    resp.body() = "Invalid method";                                            // body
    resp.content_length(resp.body().size());                            // response size
    resp.keep_alive(req.keep_alive());                                     // keep connection

    return resp;
}

template <typename RequestHandler>
void HandleConnection(tcp::socket& socket, RequestHandler&& handle_request) {
    try {
        // session buffer
        beast::flat_buffer buffer;

        // parse requests whey they are coming
        while (auto request = ReadRequest(socket, buffer)) {
            DumpRequest(*request);
            StringResponse response = handle_request(std::move(*request));
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
    const auto ADDRESS = net::ip::make_address("0.0.0.0");
    constexpr unsigned short PORT = 8080;

    net::io_context io_context;
    tcp::acceptor acceptor(io_context, tcp::endpoint{ADDRESS, PORT});
    std::cout << "Server has started..." << std::endl;

    // handling connections
    // ReSharper disable once CppDFAEndlessLoop
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
