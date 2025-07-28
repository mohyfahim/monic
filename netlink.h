#pragma once

#include <atomic>
#include <memory>
#include <mutex>

#include "database.hpp"

int monic_netlink_task(std::shared_ptr<monic::state_t> state_ptr,
                       std::atomic<bool> *shutdown_requested_ptr,
                       std::mutex *mtx_ptr);
int monic_kmsg_task(std::shared_ptr<monic::state_t> state_ptr,
                    std::atomic<bool> *shutdown_requested_ptr,
                    std::mutex *mtx_ptr);