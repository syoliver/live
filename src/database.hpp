#pragma once

#include "measurement.hpp"
#include <rocksdb/db.h>

namespace live::storage
{
  class database;

  class batch_writer
  {
   public:
    batch_writer(rocksdb::DB* db) : db_(db) {}
    batch_writer() = delete;
    batch_writer(const batch_writer& ) = delete;
    batch_writer& operator=(const batch_writer&) = delete;
    batch_writer(batch_writer&& other) noexcept
        : db_(other.db_)
        , batch_(std::move(other.batch_))
        , commited_(other.commited_)
        , rollbacked_(other.rollbacked_)
    {
      other.rollbacked_ = true;
    }

    ~batch_writer()
    {
      if(commited_ == false)
      {
        rollback();
      }
    }


    void put(const live::measurement_t& measure);
    bool commit();
    void rollback();

   private:
    rocksdb::DB*        db_;
    rocksdb::WriteBatch batch_;
    bool                commited_ = false;
    bool                rollbacked_ = false;
  };

  class database
  {
   public:
    ~database();

    bool         open(const std::string& path);
    batch_writer write_batch() { return batch_writer(db_); }

   private:
    rocksdb::DB* db_ = nullptr;
  };
} // namespace live::storage