#pragma once
#include "sdk.h"
#define BOOST_BEAST_USE_STD_STRING_VIEW

#include <iostream>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio/strand.hpp>
#include <memory>
#include <utility>

#include "logging.h"

namespace http_server {

inline void ReportError(const boost::beast::error_code &ec, const std::string_view what) {
    json::value error_data = {
        {"code", ec.value()},
        {"text", ec.message()},
        {"where", std::string(what)}
    };
    BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, error_data) << "error";
}

namespace net = boost::asio;
using tcp = net::ip::tcp;
namespace beast = boost::beast;
namespace http = beast::http;

class SessionBase {
protected:
    using HttpRequest = http::request<http::string_body>;

    ~SessionBase() = default;
    explicit SessionBase(tcp::socket&& socket) : stream_(std::move(socket)) {}

    template <typename Body>
    void Write(http::response<Body>&& response);

public:
    SessionBase(const SessionBase&) = delete;
    SessionBase& operator=(const SessionBase&) = delete;
    void Run();

protected:
    void Read();
    void OnRead(const boost::system::error_code &ec, std::size_t bytes_read);
    void Close();
    void OnWrite(bool close, const boost::system::error_code &ec, std::size_t bytes_written);
    virtual void HandleRequest(HttpRequest &&request) = 0;
    virtual std::shared_ptr<SessionBase> GetSharedThis() = 0;

    beast::tcp_stream stream_;
    beast::flat_buffer buffer_;
    HttpRequest request_;
};

template <typename RequestHandler>
class Session final : public SessionBase, public std::enable_shared_from_this<Session<RequestHandler>> {
public:
    template <typename Handler>
    Session(tcp::socket&& socket, Handler&& request_handler)
        : SessionBase(std::move(socket))
        , request_handler_(std::forward<Handler>(request_handler)) {
    }

private:
    void HandleRequest(HttpRequest&& request) override {
        auto remote_ip = stream_.socket().remote_endpoint().address().to_string();
        request_handler_(std::move(request), [self = this->shared_from_this()](auto&& response) {
            self->Write(std::move(response));
        }, remote_ip);
    }

    std::shared_ptr<SessionBase> GetSharedThis() override {
        return this->shared_from_this();
    }

    RequestHandler request_handler_;
};

template <typename RequestHandler>
class Listener : public std::enable_shared_from_this<Listener<RequestHandler>> {
public:
    template <typename Handler>
    Listener(net::io_context& ioc, const tcp::endpoint& endpoint, Handler&& request_handler)
        : request_handler_(std::forward<Handler>(request_handler)),
    acceptor_(net::make_strand(ioc)), io_context_(ioc) {

        acceptor_.open(endpoint.protocol());
        acceptor_.set_option(net::socket_base::reuse_address(true));
        acceptor_.bind(endpoint);
        acceptor_.listen(net::socket_base::max_listen_connections);
    }

    void Run() { DoAccept(); }

private:
    void DoAccept() {
        acceptor_.async_accept(
            net::make_strand(io_context_),
            beast::bind_front_handler(&Listener::OnAccept, this->shared_from_this()));
    }

    void OnAccept(const boost::system::error_code &ec, tcp::socket socket) {
        if (ec) {
            return ReportError(ec, "accept");
        }
        AsyncRunSession(std::move(socket));

        DoAccept();
    }

    void AsyncRunSession(tcp::socket&& socket) {
        std::make_shared<Session<RequestHandler>>(std::move(socket), request_handler_)->Run();
    }

    RequestHandler request_handler_;
    tcp::acceptor acceptor_;
    net::io_context& io_context_;
};

template <typename RequestHandler>
void ServeHttp(net::io_context& ioc, const tcp::endpoint& endpoint, RequestHandler&& handler) {
    std::make_shared<Listener<RequestHandler>>(ioc, endpoint, std::forward<RequestHandler>(handler))->Run();
}
}
