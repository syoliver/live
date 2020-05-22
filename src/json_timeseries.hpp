#pragma once

#include "database.hpp"
#include <string>

namespace live::http
{
  class stream;
}

namespace live::timeseries
{
  class json_storage
  {
   public:
    json_storage(live::http::stream& cs) : cs_(cs) {}
    bool write_to(live::storage::database& storage) &&;

   private:
    live::http::stream& cs_;
  };

  class json
  {
   public:
    json() {}
    json(const json&) = delete;

    json_storage from(live::http::stream& cs) { return json_storage(cs); }
  };
} // namespace live::timeseries