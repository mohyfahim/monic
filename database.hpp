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

inline auto monic_database_setup() {

  std::string file_path = "/usr/share/monic/db.sqlite";
  struct stat st;

  if (stat(file_path.c_str(), &st) == 0) {
    constexpr off_t max_db_size = 4 * 1024 * 1024; // 4 MB
    if (st.st_size > max_db_size) {
      log_info("Database file exceeds %ld bytes (%ld bytes). Deleting: %s",
               max_db_size, st.st_size, file_path.c_str());
      if (std::remove(file_path.c_str()) != 0) {
        log_error("Failed to delete database file: %s", file_path.c_str());
      }
    }
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