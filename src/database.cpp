#include "database.hpp"
#include <rocksdb/db.h>
#include <rocksdb/write_batch.h>
#include <variant>

namespace
{
  // helper type for the visitor #4
  template <class... Ts>
  struct overloaded : Ts...
  {
    using Ts::operator()...;
  };
  // explicit deduction guide (not needed as of C++20)
  template <class... Ts>
  overloaded(Ts...) -> overloaded<Ts...>;

  auto slice_measurement_value(const ::live::measurement_t& measure)
  {
    const void* pointer = nullptr;
    std::size_t size    = 0;
    std::visit(overloaded{[&pointer, &size](const std::string& value) {
                            pointer = value.data();
                            size    = value.size();
                          },
                          [&pointer, &size](auto value) {
                            pointer = &value;
                            size    = sizeof(value);
                          }},
               measure.value);

    return std::make_tuple(reinterpret_cast<const char*>(pointer), size);
  }
} // namespace

namespace live::storage
{
  void batch_writer::put(const live::measurement_t& measure)
  {
    const auto& impl_value  = slice_measurement_value(measure);
    const auto  index_value = measure.value.index();

    rocksdb::Slice key[] = {
      measure.name,
      rocksdb::Slice(reinterpret_cast<const char*>(&measure.timestamp), sizeof(measure.timestamp))
    };

    rocksdb::Slice value[] = {
      rocksdb::Slice(reinterpret_cast<const char*>(&index_value), sizeof(index_value)),
      rocksdb::Slice(std::get<0>(impl_value), std::get<1>(impl_value))
    };

    batch_.Put(rocksdb::SliceParts(key, 2), rocksdb::SliceParts(value, 2));
  }

  bool batch_writer::commit()
  {
    if(rollbacked_ == false && commited_ == false)
    {
      auto status = db_->Write(rocksdb::WriteOptions(), &batch_);
      if(status.ok())
      {
        commited_ = true;
      }
    }
    return commited_;
  }

  void batch_writer::rollback()
  {
    if(rollbacked_ == false && commited_ == false)
    {
      rollbacked_ = true;
    }
  }

  bool database::open(const std::string& path)
  {
    rocksdb::Options options;
    options.create_if_missing = true;
    rocksdb::Status status    = rocksdb::DB::Open(options, path, &db_);
    return status.ok();
  }

  database::~database() { delete db_; }

} // namespace live::storage