#include "server.hpp"
#include "request_handler.hpp"
#include <boost/asio/io_context_strand.hpp>
#include <boost/asio/ip/address.hpp>
#include <deque>
#include <functional>
#include <iostream>
#include <optional>
#include <tuple>


namespace live::http
{
  server::server(boost::asio::io_context&                     ioc,
                 std::shared_ptr<live::http::request_handler> handler,
                 std::uint64_t                                number_of_threads)
      : ioc_(ioc)
      , pool_(ioc, number_of_threads)
      , handler_(handler)
      , post_strand_(ioc)
  {
  }

  void server::listen(const std::string& address, std::uint16_t port)
  {
    acceptor_.emplace(ioc_, boost::asio::ip::tcp::endpoint(boost::asio::ip::make_address(address), port));

    for(std::uint64_t i = 0; i < 2 * pool_.number_of_threads() + 1; ++i)
    {
      auto& worker = workers_.emplace_back(ioc_, acceptor_.value(), handler_, post_strand_);
      worker.start();
    }
  }
} // namespace live::rest