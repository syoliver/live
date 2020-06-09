#include "worker.hpp"
#include <boost/asio/io_context_strand.hpp>
#include <boost/coroutine2/coroutine.hpp>

namespace live::http
{
  worker::worker(boost::asio::io_context&                     ioc,
                 boost::asio::ip::tcp::acceptor&              acceptor,
                 std::shared_ptr<live::http::request_handler> handler,
                 boost::asio::io_context::strand&             post_strand)
      : acceptor_(acceptor)
      , handler_(handler)
      , post_strand_(post_strand)
      , ioc_(ioc)
  {
  }

  void worker::start()
  {
    accept();
    // check_deadline();
  }

  void worker::accept()
  {
    // Clean up any previous connection.
    boost::beast::error_code ec;
    socket_.close(ec);
    buffer_.consume(buffer_.size());

    acceptor_.async_accept(socket_, [this](boost::beast::error_code ec) {
      if(ec)
      {
        accept();
      }
      else
      {
        // Request must be fully processed within 60 seconds.
        request_deadline_.expires_after(std::chrono::seconds(60));

        read_request();
      }
    });
  }

  void worker::read_request()
  {
    // On each read the parser needs to be destroyed and
    // recreated. We store it in a boost::optional to
    // achieve that.
    //
    // Arguments passed to the parser constructor are
    // forwarded to the message object. A single argument
    // is forwarded to the body constructor.
    //
    // We construct the dynamic body with a 1MB limit
    // to prevent vulnerability to buffer attacks.
    //
    parser_.emplace(std::piecewise_construct, std::make_tuple(), std::make_tuple(alloc_));

    auto& parser = *parser_;

    boost::beast::error_code ec;
    boost::beast::http::read_header(socket_, buffer_, parser, ec);

    if(ec)
      return;

    boost::asio::post(boost::asio::bind_executor(post_strand_, [this, &parser]() {
      using coroutine_t = boost::coroutines2::coroutine<std::tuple<char*, std::size_t, boost::beast::error_code>>;
      using pull_t      = coroutine_t::pull_type;
      using push_t      = coroutine_t::push_type;

      auto  stream     = std::make_unique<live::http::coroutine::stream<pull_t>>();
      auto& stream_ref = *stream.get();

      live::http::stream_request request(parser.get().method(), parser.get().target(), std::move(stream));

      char   buf[1024];
      bool   handle_request_done = false;
      push_t writer([this, &stream_ref, &request, &handle_request_done](pull_t& in) {
        stream_ref.set_stream_puller(in);
        const auto& response = handler_->handle(request);
        handle_request_done  = true;

        send_response(response);
      });

      boost::beast::error_code ec;
      while(!parser.is_done() && !handle_request_done)
      {
        parser.get().body().data = buf;
        parser.get().body().size = sizeof(buf);
        boost::beast::http::read(socket_, buffer_, parser, ec);

        writer(std::make_tuple(buf, sizeof(buf) - parser.get().body().size, ec));

        if(ec != boost::beast::http::error::need_buffer)
        {
          return;
        }
      }
    }));
  }

  void worker::send_response(live::http::response response)
  {
    string_response_.emplace(std::piecewise_construct, std::make_tuple(), std::make_tuple(alloc_));

    string_response_->result(response.status());
    string_response_->keep_alive(false);
    string_response_->set(boost::beast::http::field::server, "Beast");
    string_response_->set(boost::beast::http::field::content_type, "text/plain");
    // string_response_->body() = error;
    string_response_->prepare_payload();

    string_serializer_.emplace(*string_response_);

    boost::beast::http::async_write(socket_, string_serializer_.value(), [this](boost::beast::error_code ec, std::size_t) {
      socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);
      string_serializer_.reset();
      string_response_.reset();
      accept();
    });
  }
  void worker::check_deadline(boost::beast::error_code ec)
  {
    if(!ec)
    {
      // The deadline may have moved, so check it has really passed.
      if(request_deadline_.expiry() <= std::chrono::steady_clock::now())
      {
        // Close socket to cancel any outstanding operation.
        socket_.close();

        // Sleep indefinitely until we're given a new deadline.
        request_deadline_.expires_at(std::chrono::steady_clock::time_point::max());
      }
      request_deadline_.async_wait([this](boost::beast::error_code ec) { check_deadline(ec); });
    }
  }

} // namespace live::http
