#include "database.hpp"
#include "json_timeseries.hpp"
#include "http/request_handler.hpp"
#include "http/server.hpp"
#include "http/stream_request.hpp"
#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <thread>
#include <iostream>

void handle_shudown(boost::asio::io_context& ioc)
{
  boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
  signals.async_wait([&ioc](const boost::system::error_code& ec, int signal_number) {
    if(signal_number != 0)
    {
      ioc.stop();
    }
  });
}

class request_handler : public live::http::request_handler
{
 public:
  request_handler(live::storage::database& database) : database_(database) {}

  virtual live::http::response handle(const live::http::stream_request& request) override
  {
    if(live::timeseries::json().from(request.body_stream()).write_to(database_))
    {
      return live::http::response(boost::beast::http::status::ok);
    }
    return live::http::response(boost::beast::http::status::internal_server_error);
  }

 private:
  live::storage::database& database_;
};

int main()
{
  boost::asio::io_context ioc;
  handle_shudown(ioc);

  live::storage::database database;
  live::http::server      server(ioc, std::make_shared<request_handler>(database), 0);

  server.listen("0.0.0.0", 80);
  ioc.run();

  return 0;
}