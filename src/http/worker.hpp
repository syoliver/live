#pragma once

#include "fields_alloc.hpp"
#include "request_handler.hpp"
#include "stream_request.hpp"
#include <boost/asio/io_context.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <optional>
#include <memory>

namespace live::http
{
  class worker
  {
   public:
    worker(const worker&) = delete; 
    worker& operator=(worker const&) = delete;

    worker(boost::asio::io_context&                     ioc,
           boost::asio::ip::tcp::acceptor&              acceptor,
           std::shared_ptr<live::http::request_handler> handler,
           boost::asio::io_context::strand&             post_strand);

    void start();

   private:
    void accept();
    void read_request();
    void send_response(live::http::response response);
    void check_deadline(boost::beast::error_code ec = boost::beast::error_code{});

   private:
    using alloc_t = fields_alloc<char>;

    boost::asio::io_context& ioc_;

    // The acceptor used to listen for incoming connections.
    boost::asio::ip::tcp::acceptor& acceptor_;

    // The socket for the currently connected client.
    boost::asio::ip::tcp::socket socket_{ioc_};

    // The buffer for performing reads
    boost::beast::flat_static_buffer<8192> buffer_;

    // The allocator used for the fields in the request and reply.
    alloc_t alloc_{8192};

    // The parser for reading the requests
    using request_body_t = boost::beast::http::buffer_body;
    std::optional<boost::beast::http::request_parser<request_body_t, alloc_t>> parser_;

    boost::asio::basic_waitable_timer<std::chrono::steady_clock> request_deadline_{
      ioc_,
      (std::chrono::steady_clock::time_point::max)()};

    boost::asio::io_context::strand&             post_strand_;
    std::shared_ptr<live::http::request_handler> handler_;

    // The string-based response message.
    boost::optional<
      boost::beast::http::response<boost::beast::http::string_body, boost::beast::http::basic_fields<alloc_t>>>
      string_response_;

    // The string-based response serializer.
    boost::optional<boost::beast::http::response_serializer<boost::beast::http::string_body,
                                                            boost::beast::http::basic_fields<alloc_t>>>
      string_serializer_;
  };

} // namespace live::http
