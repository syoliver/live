#pragma once

#include "response.hpp"

namespace live::http
{
  class stream_request;

  class request_handler
  {
   public:
    virtual live::http::response handle(const live::http::stream_request&) = 0;
  };
} // namespace live::http