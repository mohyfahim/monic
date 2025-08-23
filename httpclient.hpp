#pragma once

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>

#include "log.h"
#include "monic_shared.h"

int monic_sync_task(std::shared_ptr<monic::state_t> state_ptr,
                    std::condition_variable *shutdown_requested_ptr,
                    std::mutex *data_mtx_ptr, std::mutex *cv_mtx_ptr) {
  log_debug("sync task started");

  while (true) {
    {

      {
        std::lock_guard<std::mutex> lock(*data_mtx_ptr);

        auto records = state_ptr->storage->get_all<SampleModel>(
            sqlite_orm::where(sqlite_orm::c(&SampleModel::synced) == 0),
            sqlite_orm::order_by(&SampleModel::id), sqlite_orm::limit(10));
        for (auto &record : records) {
          log_debug("record: ", state_ptr->storage->dump(record).c_str());
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