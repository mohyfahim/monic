#include <optional>
#include <string>

#include "sqlite_orm.h"

typedef enum { INTERFACE_UP, INTERFACE_DOWN } interface_status_t;

struct SampleModel {
  int id;
  bool connection_status;
  std::optional<std::string> interface;
  std::optional<std::string> ip;
  interface_status_t interface_status;
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
          sqlite_orm::make_column("INTERFACE", &SampleModel::interface),
          sqlite_orm::make_column("IP", &SampleModel::ip),
          sqlite_orm::make_column("INTERFACE_STATUS",
                                  &SampleModel::interface_status)));
}

using monic_storage_t = decltype(monic_database_setup());