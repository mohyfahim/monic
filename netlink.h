#pragma once

#include <atomic>

int monic_netlink_task(std::atomic<bool> *shutdown_requested_ptr);
