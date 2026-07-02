#include "http_server.h"

#include <boost/asio/dispatch.hpp>

namespace http_server {

///////////////////////////////////////////////
// SessionBase
///////////////////////////////////////////////

void SessionBase::Run() {
    net::dispatch(stream_.get_executor(), beast::bind_front_handler(&SessionBase::Read, GetSharedThis()));
}

void SessionBase::Read() {
    request_ = {};
    stream_.expires_after(std::chrono::seconds(30));
    http::async_read(stream_, buffer_, request_, beast::bind_front_handler(&SessionBase::OnRead, GetSharedThis()));
}

void SessionBase::OnRead(const boost::system::error_code &ec, size_t bytes_read) {
    if (ec == http::error::end_of_stream) {
        return Close();
    }
    if (ec) {
        return ReportError(ec, "read");
    }
    HandleRequest(std::move(request_));
}

void SessionBase::Close() {
    boost::system::error_code ec;
    stream_.socket().shutdown(tcp::socket::shutdown_send, ec);
    if (ec) {
        return ReportError(ec, "shutdown");
    }
}

void SessionBase::Write(http::response<http::string_body> &&response) {
    auto safe_response = std::make_shared<http::response<http::string_body>>(std::move(response));
    auto self = GetSharedThis();
    http::async_write(
        stream_, *safe_response,
        [safe_response, self](const beast::error_code &ec, const std::size_t bytes_written) {
            self->OnWrite(safe_response->need_eof(), ec, bytes_written);
        });
}

void SessionBase::OnWrite(
    const bool close,
    const boost::system::error_code &ec,
    [[maybe_unused]] std::size_t bytes_written
    ) {
    if (ec) {
        return ReportError(ec, "write");
    }
    if (close) {
        return Close();
    }
    Read();
}
}  // namespace http_server
