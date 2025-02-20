//
//  basic_db.cc
//  YCSB-C
//
//  Created by Jinglei Ren on 12/17/14.
//  Copyright (c) 2014 Jinglei Ren <jinglei@ren.systems>.
//

#include "db/db_factory.h"

#include <string>
#include "db/basic_db.h"
#include "db/lock_stl_db.h"
#include "db/redis_db.h"
#include "db/rocks_db.h"
#include "db/tbb_rand_db.h"
#include "db/tbb_scan_db.h"

using namespace std;
using ycsbc::DB;
using ycsbc::DBFactory;

DB* DBFactory::CreateDB(utils::Properties &props,
                        bool use_existing_db) {
  if (use_existing_db && props["dbname"] != "rocksdb") {
    throw utils::Exception("Only `-dbname rocksdb` supports running on a "
                           "prepopulated DB");
  }
  if (props["dbname"] == "basic") {
    return new BasicDB;
  } else if (props["dbname"] == "lock_stl") {
    return new LockStlDB;
  } else if (props["dbname"] == "redis") {
    int port = stoi(props["port"]);
    int slaves = stoi(props["slaves"]);
    return new RedisDB(props["host"].c_str(), port, slaves);
  } else if (props["dbname"] == "tbb_rand") {
    return new TbbRandDB;
  } else if (props["dbname"] == "tbb_scan") {
    return new TbbScanDB;
  } else if (props["dbname"] == "rocksdb") {
    std::string dir = props.GetProperty("rocksdb_dir");
    if (dir.empty()) {
      throw utils::Exception("`-rocksdb_dir` is required with "
                             "`-dbname rocksdb`");
    }
    std::string options_file = props.GetProperty("rocksdb_options_file");
    return new RocksDB(dir, options_file, use_existing_db);
  } else return NULL;
}

