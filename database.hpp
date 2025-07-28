#pragma once

#include <cstdio>
#include <optional>
#include <string>
#include <sys/stat.h>

#include "log.h"
#include "sqlite_orm.h"

struct SampleModel {
  int id;
  std::optional<bool> connection_status;
  std::optional<std::string> interface;
  std::optional<std::string> ip;
  std::optional<std::string> interface_status;
  long created_at;
  bool synced;
};

long long get_file_size(const std::string &filename) {
  struct stat st;
  if (stat(filename.c_str(), &st) == 0) {
    return st.st_size;
  }
  return -1; // Indicate error
}

inline auto monic_database_setup() {

  std::string file_path = "/usr/share/monic/db.sqlite";

  long long size = get_file_size(file_path);
  if (size > 4 * 1024 * 1024) {
    log_info("delete database: %ld BYTES", size);
    std::remove(file_path.c_str());
  }

  return sqlite_orm::make_storage(
      file_path,
      sqlite_orm::make_table(
          "SAMPLE",
          sqlite_orm::make_column("ID", &SampleModel::id,
                                  sqlite_orm::primary_key()),
          sqlite_orm::make_column("CONNECTION_STATUS",
                                  &SampleModel::connection_status),
          sqlite_orm::make_column("SYNCED", &SampleModel::synced),
          sqlite_orm::make_column("INTERFACE", &SampleModel::interface),
          sqlite_orm::make_column("IP", &SampleModel::ip),
          sqlite_orm::make_column("INTERFACE_STATUS",
                                  &SampleModel::interface_status),
          sqlite_orm::make_column("CREATED_AT", &SampleModel::created_at)));
}

using monic_storage_t = decltype(monic_database_setup());

namespace monic {

typedef struct {
  bool connect;
  monic_storage_t *storage;
} state_t;

inline long get_current_epoch() {
  auto duration = std::chrono::system_clock::now().time_since_epoch();
  return std::chrono::duration_cast<std::chrono::seconds>(duration).count();
};

} // namespace monic