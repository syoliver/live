#include "json_timeseries.hpp"
#include "database.hpp"
#include "http/stream_request.hpp"
#include <rapidjson/reader.h>
#include <rapidjson/error/en.h>
#include <boost/beast/core/error.hpp>
#include <cstdint>
#include <iostream>
#include <optional>
#include <variant>
/*
[{
  "measurement": "name",
  "value": 42,
  "timestamp": 1234,
}]
*/

struct timeseries_json_sax_handler : public rapidjson::BaseReaderHandler<rapidjson::UTF8<>, timeseries_json_sax_handler>
{
  timeseries_json_sax_handler(live::storage::batch_writer& storage) : storage_(storage) {}

  bool Null() { return false; }
  bool Bool(bool b)
  {
    if(in_field == field_t::value)
    {
      measurement.value     = b;
      measurement_has_value = true;
      return true;
    }
    return false;
  }

  bool Int(int i)
  {
    if(in_field == field_t::value)
    {
      measurement.value     = i;
      measurement_has_value = true;
      return true;
    }
    else if(in_field == field_t::timestamp)
    {
      if(i > 0)
      {
        measurement.timestamp     = i;
        measurement_has_timestamp = true;
        return true;
      }
    }
    return false;
  }

  bool Uint(unsigned u)
  {
    if(in_field == field_t::value)
    {
      measurement.value     = u;
      measurement_has_value = true;
      return true;
    }
    else if(in_field == field_t::timestamp)
    {
      measurement.timestamp     = u;
      measurement_has_timestamp = true;
      return true;
    }

    return false;
  }

  bool Int64(int64_t i)
  {
    if(in_field == field_t::value)
    {
      measurement.value     = i;
      measurement_has_value = true;
      return true;
    }
    else if(in_field == field_t::timestamp)
    {
      if(i > 0)
      {
        measurement.timestamp     = i;
        measurement_has_timestamp = true;
        return true;
      }
    }
    return false;
  }

  bool Uint64(uint64_t u)
  {
    if(in_field == field_t::value)
    {
      measurement.value     = u;
      measurement_has_value = true;
      return true;
    }
    else if(in_field == field_t::timestamp)
    {
      measurement.timestamp     = u;
      measurement_has_timestamp = true;
      return true;
    }

    return false;
  }

  bool Double(double d)
  {
    if(in_field == field_t::value)
    {
      measurement.value     = d;
      measurement_has_value = true;
      return true;
    }
    return false;
  }
  bool String(const char* str, rapidjson::SizeType length, bool copy)
  {
    if(in_field == field_t::value)
    {
      measurement.value     = std::string(str, length);
      measurement_has_value = true;
      return true;
    }
    else if(in_field == field_t::name)
    {
      measurement.name     = std::string(str, length);
      measurement_has_name = true;
      return true;
    }

    return false;
  }
  bool StartObject()
  {
    measurement               = live::measurement_t{};
    measurement_has_name      = false;
    measurement_has_value     = false;
    measurement_has_timestamp = false;
    if(level != 1)
    {
      return false;
    }
    ++level;
    return true;
  }

  bool Key(const char* str, rapidjson::SizeType length, bool copy)
  {
    if(level != 2)
    {
      return false;
    }
    in_field = field_t::none;
    if(std::memcmp(str, "measurement", length) == 0)
    {
      in_field = field_t::name;
    }
    else if(std::memcmp(str, "value", length) == 0)
    {
      in_field = field_t::value;
    }
    else if(std::memcmp(str, "timestamp", length) == 0)
    {
      in_field = field_t::timestamp;
    }
    else
    {
      return false;
    }

    return true;
  }

  bool EndObject(rapidjson::SizeType memberCount)
  {
    --level;

    in_field = field_t::none;
    if(measurement_has_name && measurement_has_value && measurement_has_timestamp)
    {
      // if object is complete : push to database
      storage_.put(measurement);
      return true;
    }

    return false;
  }

  bool StartArray()
  {
    if(level > 0)
    {
      return false;
    }
    ++level;
    return true;
  }
  bool EndArray(rapidjson::SizeType elementCount)
  {
    --level;
    return true;
  }

  live::storage::batch_writer& storage_;

  int  level          = 0;
  bool in_list        = false;
  bool in_measurement = false;

  enum class field_t
  {
    none,
    name,
    value,
    timestamp
  };

  field_t in_field = field_t::none;

  live::measurement_t measurement;
  bool                measurement_has_value;
  bool                measurement_has_name;
  bool                measurement_has_timestamp;
};

class stream_reader
{
 public:
  using Ch = char;

  stream_reader(live::http::stream& is) : is_(is), buffer_(1024, 0), offset_(0), end_(0), tellg_(0) {}

  stream_reader(const stream_reader&) = delete;
  stream_reader& operator=(const stream_reader&) = delete;

  Ch Peek() const
  {
    auto ec = LoadFromStream();
    if(!ec)
    {
      return buffer_[offset_];
    }
    return '\0';
  }

  Ch Take()
  {
    auto ec = LoadFromStream();
    if(!ec)
    {
      tellg_++;
      return buffer_[offset_++];
    }
    return '\0';
  }

  size_t Tell() const { return tellg_; }

  Ch* PutBegin()
  {
    assert(false);
    return 0;
  }
  void   Put(Ch) { assert(false); }
  void   Flush() { assert(false); }
  size_t PutEnd(Ch*)
  {
    assert(false);
    return 0;
  }

 private:
  boost::beast::error_code LoadFromStream() const
  {
    boost::beast::error_code ec;
    if(offset_ >= end_)
    {
      offset_ = 0;
      end_    = 0;
      while(end_ == 0 && !ec)
      {
        std::fill(std::begin(buffer_), std::end(buffer_), '\0');
        end_ = is_.readsome(&buffer_[0], buffer_.size(), ec);
      }
    }
    return ec;
  }

 private:
  live::http::stream&     is_;
  mutable std::vector<Ch> buffer_;
  mutable std::size_t     offset_;
  mutable std::size_t     end_;
  std::size_t             tellg_;
};

namespace live::timeseries
{
  bool json_storage::write_to(live::storage::database& storage) &&
  {
    auto                        writer = storage.write_batch();
    timeseries_json_sax_handler handler(writer);
    rapidjson::Reader           reader;
    stream_reader               ss(cs_);
    rapidjson::ParseResult      result = reader.Parse(ss, handler);
    if(result.IsError() == false)
    {
      return writer.commit();
    }
    std::cerr << rapidjson::GetParseError_En(result.Code()) << std::endl;
    return false;
  }
} // namespace live::timeseries