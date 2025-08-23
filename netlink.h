#pragma once

#include <atomic>
#include <memory>
#include <mutex>

#include "database.hpp"

int monic_netlink_task(std::shared_ptr<monic::state_t> state_ptr,
                       std::condition_variable *shutdown_requested_ptr,
                       std::mutex *data_mtx_ptr, std::mutex *cv_mtx_ptr);
int monic_kmsg_task(std::shared_ptr<monic::state_t> state_ptr,
                    std::condition_variable *shutdown_requested_ptr,
                    std::mutex *data_mtx_ptr, std::mutex *cv_mtx_ptr);