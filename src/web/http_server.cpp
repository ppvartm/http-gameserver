#include "http_server.h"


namespace http_server {

    //SessinBase part ********************************************
	void SessionBase::Run() {
		net::dispatch(stream_.get_executor(), beast::bind_front_handler(&SessionBase::Read, GetSharedThis()));
	}
    void SessionBase::Read() {
        request_ = {};
        stream_.expires_after(30s);
        http::async_read(stream_, buffer_, request_,
            beast::bind_front_handler(&SessionBase::OnRead, GetSharedThis()));
    }
    void SessionBase::OnRead(beast::error_code ec, [[maybe_unused]] std::size_t bytes_read) {
        if (ec == http::error::end_of_stream) 
           return Close();
        if (ec) {
            ServerErrorLog(ec.value(), ec.message(), "read");
            return;
        }
        HandleRequest(std::move(request_));
    }
    void SessionBase::OnWrite(bool close, beast::error_code ec, [[maybe_unused]] std::size_t bytes_written) {
        if (ec) {
            ServerErrorLog(ec.value(), ec.message(), "write");
            return;
        }
        if (close) 
           return Close();
        Read();
    }
    void SessionBase::Close() {
        beast::error_code ec;
        stream_.socket().shutdown(tcp::socket::shutdown_send, ec);
        if (ec) 
          return;
    }
    // ********************************************




}  // namespace http_server
