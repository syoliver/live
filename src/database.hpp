#pragma once

#include "measurement.hpp"

namespace live::storage
{
  class database;

  class batch_writer
  {
   public:
    batch_writer(database* db) : db_(db), commited_(false) {}
    ~batch_writer()
    {
      if(commited_ == false)
      {
        rollback();
      }
    }
    void put(const live::measurement& measure);
    bool commit();
    void rollback();

   private:
    database* db_;
    bool      commited_;
  };

  class database
  {
   public:
    batch_writer write_batch() { return batch_writer(this); }
  };
} // namespace live::storage