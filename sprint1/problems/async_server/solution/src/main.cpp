#include "sdk.h"
//
#include <boost/asio/signal_set.hpp>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

#include "http_server.h"

namespace {
namespace net = boost::asio;
namespace sys = boost::system;
namespace http = boost::beast::http;

using StringRequest = http::request<http::string_body>;
using StringResponse = http::response<http::string_body>;

struct ContentType {
    ContentType() = delete;
    constexpr static std::string_view TEXT_HTML = "text/html";
};

StringResponse MakeStringResponse(
    const http::status status,
    const std::string_view body,
    const unsigned http_version,
    const bool keep_alive,
    const std::string_view content_type = ContentType::TEXT_HTML
) {
    StringResponse response(status, http_version);
    response.set(http::field::content_type, content_type);
    response.body() = body;
    response.content_length(body.size());
    response.keep_alive(keep_alive);
    return response;
}

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
    resp.body() = "Invalid method";                                             // body
    resp.content_length(resp.body().size());                            // response size
    resp.keep_alive(req.keep_alive());                                     // keep connection

    return resp;
}

template <typename Fn>
void RunWorkers(unsigned n, const Fn& fn) {
    n = std::max(1u, n);
    std::vector<std::jthread> workers;
    workers.reserve(n - 1);
    while (--n) {
        workers.emplace_back(fn);
    }
    fn();
}

}  // namespace

int main() {
    const unsigned num_threads = std::thread::hardware_concurrency();

    net::io_context ioc(static_cast<int>(num_threads));

    net::signal_set signals(ioc, SIGINT, SIGTERM);
    signals.async_wait([&ioc](const sys::error_code& ec, [[maybe_unused]] int signal_number) {
        if (!ec) {
            ioc.stop();
        }
    });

    const auto address = net::ip::make_address("0.0.0.0");
    constexpr net::ip::port_type port = 8080;

    http_server::ServeHttp(ioc, {address, port}, []<typename Req>(Req&& req, auto&& sender) {
        sender(HandleRequest(std::forward<Req>(req)));
    });

    std::cout << "Server has started..." << std::endl;

    RunWorkers(num_threads, [&ioc] {
        ioc.run();
    });
}
