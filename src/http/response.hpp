#pragma once

#include <boost/beast/http/status.hpp>

namespace live::http
{

  class response
  {
   public:
    response(boost::beast::http::status status) : status_(status) {}

    boost::beast::http::status status() const { return status_; }
    
  private:
    boost::beast::http::status status_;
  };
}