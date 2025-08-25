#pragma once

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>

#include "httplib.h"
#include "json.hpp"
#include "log.h"
#include "monic_shared.h"

#define MONIC_BACKEND_API "https://boxapi.sandpod.ir"
#define MONIC_LOG_ROUTE "/v3/device/sync/logs"
#define MONIC_JSON_TYPE "application/json"

using json = nlohmann::json;

int monic_sync_task(std::shared_ptr<monic::state_t> state_ptr,
                    std::condition_variable *shutdown_requested_ptr,
                    std::mutex *data_mtx_ptr, std::mutex *cv_mtx_ptr) {
  log_debug("sync task started");

  const char *device_token = std::getenv("MONIC_DEVICE_TOKEN");
  if (!device_token) {
    log_error("provide device token");
    return -1;
  }
  while (true) {
    {

      {
        std::lock_guard<std::mutex> lock(*data_mtx_ptr);
        json rec_list_j = json::array();
        auto records = state_ptr->storage->get_all<SampleModel>(
            sqlite_orm::where(sqlite_orm::c(&SampleModel::synced) == 0),
            sqlite_orm::order_by(&SampleModel::id), sqlite_orm::limit(10));

        if (records.size()) {
          for (auto &record : records) {
            log_debug("record: %s", state_ptr->storage->dump(record).c_str());
            json rec_j;
            rec_j["connectionStatus"] = record.connection_status.value_or(0);
            std::stringstream network_status;
            network_status << record.interface.value_or("") << ","
                           << record.interface_status.value_or("") << ","
                           << record.ip.value_or("");
            rec_j["networkStatus"] = network_status.str();
            rec_j["temperature"] = record.temperature.value_or(0.0);
            rec_j["createdAt"] = record.created_at;
            rec_list_j.push_back(rec_j);
            record.synced = true;
          }
          json rec_j;
          rec_j["logs"] = rec_list_j;
          log_debug("json: %s", rec_j.dump().c_str());

          httplib::Client cli(MONIC_BACKEND_API);
          cli.set_follow_location(true);
          httplib::Result res =
              cli.Post(MONIC_LOG_ROUTE,
                       httplib::Headers({{"device-token", device_token}}),
                       rec_j.dump(), MONIC_JSON_TYPE);
          if (!res) {
            const httplib::Error err = res.error();
            log_error("http error: %s", httplib::to_string(err));
          } else {
            log_info("http code %d ,result: %s", res->status,
                     res->body.c_str());
            if (res->status == httplib::OK_200 ||
                res->status == httplib::Created_201) {
              for (auto &record : records) {
                state_ptr->storage->update(record);
              }
            }
          }
        }
      }
      // std::this_thread::sleep_for(std::chrono::seconds(60));
    }
    std::unique_lock<std::mutex> lock(*cv_mtx_ptr);
    std::cv_status cvs =
        shutdown_requested_ptr->wait_for(lock, std::chrono::seconds(60));
    if (cvs == std::cv_status::no_timeout) {
      log_debug("recieved shutdown notify");
      break;
    }
  }
  return 0;
}