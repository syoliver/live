#pragma once

#include <cstdint>
#include <string>
#include <variant>

namespace live
{
  struct measurement_t
  {
    using value_t = std::variant<std::string, double, std::int32_t, std::uint32_t, std::int64_t, std::uint64_t, bool>;

    std::string   name;
    value_t       value;
    std::uint64_t timestamp;
  };
} // namespace live