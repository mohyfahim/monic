#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>

#include "database.hpp"
#include "httplib.h"
#include "json.hpp"
#include "log.h"
#include "main.hpp"
#include "netlink.h"
#include "tcp.h"

using json = nlohmann::json;
// Configurable test host and port for connectivity check
constexpr const char *TEST_HOST = "google.com";
constexpr int TEST_PORT = 80;

std::mutex mtx;
std::atomic<bool> g_shutdown_requested(false);

void monic_signal_handler(int signum) {
  if (signum == SIGINT || signum == SIGTERM) {
    g_shutdown_requested.store(true);
  }
}

void monic_connectivity_check_task(std::shared_ptr<monic::state_t> state_ptr,
                                   std::atomic<bool> *shutdown_requested_ptr,
                                   std::mutex *mtx_ptr) {

  while (!(*shutdown_requested_ptr).load()) {
    {
      int err1 = monic_tcp_host(const_cast<char *>(TEST_HOST), TEST_PORT);
      std::cout << "host result: " << err1 << std::endl;
      int err2 = monic_tcp_ip(const_cast<char *>("216.239.38.120"), 80);
      std::cout << "ip result: " << err2 << std::endl;
      std::lock_guard<std::mutex> lock(*mtx_ptr);
      // TODO: analyze dns issues
      state_ptr->connect = (err1 == 0 || err2 == 0);
      SampleModel sm{};
      sm.created_at = monic::get_current_epoch();
      sm.connection_status = state_ptr->connect;
      sm.synced = false;
      state_ptr->storage->insert(sm);
    }
    std::this_thread::sleep_for(std::chrono::seconds(15));
  }
}

void monic_sync_task(std::shared_ptr<monic::state_t> state_ptr,
                     std::atomic<bool> *shutdown_requested_ptr,
                     std::mutex *mtx_ptr) {

  while (!(*shutdown_requested_ptr).load()) {
    { std::lock_guard<std::mutex> lock(*mtx_ptr); }
    std::this_thread::sleep_for(std::chrono::seconds(60));
  }
}

int main() {

  std::signal(SIGINT, monic_signal_handler);
  std::signal(SIGKILL, monic_signal_handler);
  log_set_level(MONIC_LOG_LEVEL);
  log_info("program started\n");

  monic_storage_t storage = monic_database_setup();
  storage.sync_schema();

  std::shared_ptr<monic::state_t> shared_state_ptr =
      std::make_shared<monic::state_t>();
  shared_state_ptr->storage = &storage;

  std::thread task1(monic_connectivity_check_task, shared_state_ptr,
                    &g_shutdown_requested, &mtx);

  std::thread task2(monic_netlink_task, shared_state_ptr, &g_shutdown_requested,
                    &mtx);

  task1.join();
  task2.join();

  log_info("program stopped\n");

  return 0;
}