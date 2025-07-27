#pragma once

#include <optional>
#include <string>

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

  return sqlite_orm::make_storage(
      "db.sqlite",
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