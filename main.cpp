#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>

#include "database.hpp"
#include "log.h"
#include "main.hpp"
#include "netlink.h"
#include "tcp.h"

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
      // TODO: check the internet connection here
      int err = monic_tcp_host(const_cast<char *>(TEST_HOST), TEST_PORT);
      std::cout << "host result: " << err << std::endl;
      err = monic_tcp_host(const_cast<char *>("127.0.0.1"), 8080);
      std::cout << "ip result: " << err << std::endl;
      std::lock_guard<std::mutex> lock(*mtx_ptr);
      (*state_ptr).connect = true;
    }
    // TODO: sleep here
    std::this_thread::sleep_for(std::chrono::seconds(5));
  }
}

int main() {

  std::signal(SIGINT, monic_signal_handler);
  std::signal(SIGKILL, monic_signal_handler);

  log_info("program started\n");

  monic_storage_t storage = monic_database_setup();

  std::shared_ptr<monic::state_t> shared_state_ptr =
      std::make_shared<monic::state_t>();

  std::thread task1(monic_connectivity_check_task, shared_state_ptr,
                    &g_shutdown_requested, &mtx);

  std::thread task2(monic_netlink_task, &g_shutdown_requested);

  task1.join();
  task2.join();

  log_info("program stopped\n");

  return 0;
}