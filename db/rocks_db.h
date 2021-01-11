//
//  rocks_db.h
//  YCSB-C
//

#ifndef YCSB_C_ROCKS_DB_H_
#define YCSB_C_ROCKS_DB_H_

#include "core/db.h"

#include "rocksdb/db.h"

#include <string>
#include <vector>

namespace ycsbc {

class RocksDB : public DB {
 public:
  RocksDB(const std::string& dbname, const std::string& options_file);

  virtual int Read(const std::string &table, const std::string &key,
                   const std::vector<std::string> *fields,
                   std::vector<KVPair> &result) override;

  virtual int Scan(const std::string &table, const std::string &key,
                   int record_count, const std::vector<std::string> *fields,
                   std::vector<std::vector<KVPair>> &result) override;

  virtual int Update(const std::string &table, const std::string &key,
                     std::vector<KVPair> &values) override;

  virtual int Insert(const std::string &table, const std::string &key,
                     std::vector<KVPair> &values) override;

  virtual int Delete(const std::string &table,
                     const std::string &key) override;

 private:
  rocksdb::DB* db_;
};

} // ycsbc

#endif // YCSB_C_ROCKS_DB_H_
