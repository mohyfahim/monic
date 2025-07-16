#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>

#include "main.hpp"

#define MONIC_WAN_INTERFACE "wwan0"
#define MONIC_WAN_PORT "/dev/cdc-wdm0"

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

  std::cout << "program started" << std::endl;

  std::shared_ptr<monic::state_t> shared_state_ptr =
      std::make_shared<monic::state_t>();

  std::thread task1(monic_connectivity_check_task, shared_state_ptr,
                    &g_shutdown_requested, &mtx);

  task1.join();

  std::cout << "program stopped" << std::endl;

  return 0;
}