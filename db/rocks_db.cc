//
//  rocks_db.cc
//  YCSB-C
//

#include "rocks_db.h"
#include "core/core_workload.h"
#include "core/utils.h"

#include "rocksdb/iterator.h"
#include "rocksdb/utilities/options_util.h"

#include <sstream>

namespace ycsbc {

namespace {

std::vector<DB::KVPair> ParseValue(const std::string& val,
                                   const std::vector<std::string> *fields) {
  std::vector<DB::KVPair> result;
  size_t begin_idx = 0;
  while (begin_idx != std::string::npos) {
    size_t end_idx = val.find(' ', begin_idx);
    if (end_idx == std::string::npos ||
        end_idx == val.size() - 1) {
      throw utils::Exception("Found field with missing value");
    }
    std::string field_name = val.substr(begin_idx, end_idx - begin_idx);
    begin_idx = end_idx + 1;
    end_idx = val.find(' ', begin_idx);
    std::string field_val;
    if (end_idx == std::string::npos) {
      field_val = val.substr(begin_idx);
      begin_idx = end_idx;
    } else {
      field_val = val.substr(begin_idx, end_idx - begin_idx);
      begin_idx = end_idx + 1;
    }
    std::pair<std::string, std::string> result_elmt(
        std::move(field_name), std::move(field_val));
    bool include = false;
    if (fields == nullptr) {
      include = true;
    } else {
      for (const std::string& field : *fields) {
        if (result_elmt.first == field) {
          include = true;
          break;
        }
      }
    }
    if (include) {
      result.emplace_back(std::move(result_elmt));
    }
  }
  return result;
}

}  // anonymous namespace

RocksDB::RocksDB(const std::string& dbname, const std::string& options_file,
                 bool use_existing_db) {
  rocksdb::DBOptions db_opts;
  std::vector<rocksdb::ColumnFamilyDescriptor> cf_descs;
  if (!options_file.empty()) {
    rocksdb::Status s = rocksdb::LoadOptionsFromFile(
        options_file, rocksdb::Env::Default(), &db_opts, &cf_descs);
    if (!s.ok()) {
      throw utils::Exception(s.ToString());
    }
    if (cf_descs.size() != 1) {
      std::ostringstream oss;
      oss << "Expected options for 1 CF (default), found " << cf_descs.size();
      throw utils::Exception(oss.str());
    }
  } else {
    cf_descs.emplace_back();
  }
  rocksdb::Options opts(db_opts, cf_descs[0].options);

  if (!use_existing_db) {
    // The dbname may not already exist in which case we expect destroy to fail
    rocksdb::DestroyDB(dbname, opts).PermitUncheckedError();
    opts.create_if_missing = true;
  }

  rocksdb::Status s = rocksdb::DB::Open(opts, dbname, &db_);
  if (!s.ok()) {
    throw utils::Exception(s.ToString());
  }
}

int RocksDB::Read(const std::string &table, const std::string &key,
                  const std::vector<std::string> *fields,
                  std::vector<KVPair> &result) {
  if (table != ycsbc::CoreWorkload::TABLENAME_DEFAULT) {
    throw utils::Exception("Non-default table not supported with RocksDB");
  }
  std::string val;
  rocksdb::Status s = db_->Get(rocksdb::ReadOptions(), key, &val);
  if (s.IsNotFound()) {
    return DB::kErrorNoData;
  } else if (!s.ok()) {
    throw utils::Exception(s.ToString());
  }
  result = ParseValue(val, fields);
  return DB::kOK;
}

int RocksDB::Scan(const std::string &table, const std::string &key,
                  int record_count, const std::vector<std::string> *fields,
                  std::vector<std::vector<KVPair>> &result) {
  if (table != ycsbc::CoreWorkload::TABLENAME_DEFAULT) {
    throw utils::Exception("Non-default table not supported with RocksDB");
  }
  rocksdb::Iterator* iter = db_->NewIterator(rocksdb::ReadOptions());
  for (int i = 0; i < record_count; ++i) {
    if (i == 0) {
      iter->Seek(key);
    } else {
      iter->Next();
    }
    if (!iter->Valid()) {
      if (iter->status().ok()) {
        return DB::kErrorNoData;
      }
      throw utils::Exception(iter->status().ToString());
    }
    result.emplace_back(ParseValue(iter->value().ToString(), fields));
  }
  return DB::kOK;
}

int RocksDB::Update(const std::string &table, const std::string &key,
                    std::vector<KVPair> &values) {
  if (table != ycsbc::CoreWorkload::TABLENAME_DEFAULT) {
    throw utils::Exception("Non-default table not supported with RocksDB");
  }
  std::vector<KVPair> read_values;
  int read_ret = Read(table, key, nullptr /* fields */, read_values);
  if (read_ret != DB::kOK) {
    return read_ret;
  }
  std::vector<KVPair> updated_values = std::move(read_values);
  for (KVPair& update : values) {
    bool found = false;
    for (KVPair& updated_value : updated_values) {
      if (updated_value.first == update.first) {
        updated_value.second = std::move(update.second);
        found = true;
        break;
      }
    }
    if (!found) {
      return DB::kErrorNoData;
    }
  }
  return Insert(table, key, updated_values);
}

int RocksDB::Insert(const std::string &table, const std::string &key,
                    std::vector<KVPair> &values) {
  if (table != ycsbc::CoreWorkload::TABLENAME_DEFAULT) {
    throw utils::Exception("Non-default table not supported with RocksDB");
  }
  size_t len = 0;
  for (size_t i = 0; i < values.size(); ++i) {
    // Space separates field name from value and value from next field
    len += values[i].first.size() + values[i].second.size() + 1;
    if (i < values.size() - 1) {
      len++;
    }
  }
  std::string val;
  val.reserve(len);
  for (size_t i = 0; i < values.size(); ++i) {
    if (values[i].first.find(' ') != std::string::npos) {
      throw utils::Exception("Field name with space not supported");
    }
    val.append(values[i].first);
    val.append(" ");
    if (values[i].second.find(' ') != std::string::npos) {
      throw utils::Exception("Field value with space not supported");
    }
    val.append(values[i].second);
    if (i < values.size() - 1) {
      val.append(" ");
    }
  }

  rocksdb::Status s = db_->Put(rocksdb::WriteOptions(), key, val);
  if (!s.ok()) {
    throw utils::Exception(s.ToString());
  }
  return DB::kOK;
}

int RocksDB::Delete(const std::string &table,
                    const std::string &key) {
  if (table != ycsbc::CoreWorkload::TABLENAME_DEFAULT) {
    throw utils::Exception("Non-default table not supported with RocksDB");
  }
  rocksdb::Status s = db_->Delete(rocksdb::WriteOptions(), key);
  if (!s.ok()) {
    throw utils::Exception(s.ToString());
  }
  return DB::kOK;
}

} // namespace ycsbc
