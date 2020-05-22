#pragma once

#include <restinio/all.hpp>
#include <asio/io_context.hpp>

namespace live::rest
{
  class server
  {
  
  public:
    void setup();

  private:
    using router_t = restinio::router::express_router_t<>;
    router_t router_;

    asio::io_context io_;
    asio::strand     post_strand_;
  }
}