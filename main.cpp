#include <array>
#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <memory>
#include <mutex>
#include <sys/select.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

#include "database.hpp"
#include "log.h"
#include "main.hpp"
#include "netlink.h"
#include "tcp.h"

// using json = nlohmann::json;
// Configurable test host and port for connectivity check
constexpr const char *TEST_HOST = "google.com";
constexpr int TEST_PORT = 80;

std::mutex mtx;
std::atomic<bool> g_shutdown_requested(false);

void monic_signal_handler(int signum) {
  if (signum == SIGINT || signum == SIGTERM || signum == SIGKILL) {
    g_shutdown_requested.store(true);
  }
}

std::string executeCommand(const std::string &command, int timeout_ms) {
  // 1. Parse the command string into command and arguments for execvp
  std::istringstream iss(command);
  std::vector<std::string> args;
  std::string arg;
  while (iss >> arg) {
    args.push_back(arg);
  }

  if (args.empty()) {
    return ""; // No command to execute
  }

  std::vector<char *> c_args;
  for (auto &a : args) {
    c_args.push_back(&a[0]);
  }
  c_args.push_back(nullptr); // execvp expects a null-terminated array

  // 2. Create a pipe to capture the child's output
  int pipefd[2];
  if (pipe(pipefd) == -1) {
    perror("pipe");
    return "";
  }

  // 3. Fork the process
  pid_t pid = fork();
  if (pid == -1) {
    perror("fork");
    close(pipefd[0]);
    close(pipefd[1]);
    return "";
  }

  if (pid == 0) {
    // --- Child Process ---
    // Redirect stdout and stderr to the write-end of the pipe
    close(pipefd[0]); // Close unused read end
    dup2(pipefd[1], STDOUT_FILENO);
    dup2(pipefd[1], STDERR_FILENO);
    close(pipefd[1]);

    // Execute the command
    execvp(c_args[0], c_args.data());

    // execvp only returns on error
    perror("execvp");
    _exit(127); // Use _exit in child process to avoid flushing parent's buffers
  } else {
    // --- Parent Process ---
    close(pipefd[1]); // Close unused write end

    std::string output;
    std::array<char, 256> buffer;

    while (true) {
      // 4. Use select() to wait for data on the pipe with a timeout
      fd_set read_fds;
      FD_ZERO(&read_fds);
      FD_SET(pipefd[0], &read_fds);

      struct timeval timeout;
      timeout.tv_sec = timeout_ms / 1000;
      timeout.tv_usec = (timeout_ms % 1000) * 1000;

      // The select call monitors the pipe's read file descriptor
      int result = select(pipefd[0] + 1, &read_fds, nullptr, nullptr, &timeout);

      if (result == -1) { // Error
        perror("select");
        break;
      }

      if (result == 0) { // Timeout
        log_error("Command timed out.");
        kill(pid, SIGKILL);       // Forcefully terminate the child
        waitpid(pid, nullptr, 0); // Clean up the zombie process
        close(pipefd[0]);
        return ""; // Return empty string on timeout
      }

      // If we are here, select() returned > 0, so data is available to read
      ssize_t bytes_read = read(pipefd[0], buffer.data(), buffer.size());

      if (bytes_read > 0) {
        output.append(buffer.data(), bytes_read);
      } else {
        // End of file (child process finished) or an error
        break;
      }
    }

    // 5. Clean up
    waitpid(pid, nullptr, 0); // Wait for the child to fully terminate
    close(pipefd[0]);

    return output;
  }
}

void monic_connectivity_check_task(std::shared_ptr<monic::state_t> state_ptr,
                                   std::atomic<bool> *shutdown_requested_ptr,
                                   std::mutex *mtx_ptr) {

  while (!(*shutdown_requested_ptr).load()) {
    {
      int err1 = monic_tcp_host(const_cast<char *>(TEST_HOST), TEST_PORT);
      log_info("host result: %d", err1);
      int err2 = monic_tcp_ip(const_cast<char *>("216.239.38.120"), 80);
      log_info("ip result: %d", err2);

      std::string ls_output = executeCommand(
          "uqmi -d /dev/cdc-wdm0 -s -t 5000 --get-signal-info", 5000);
      log_info("output command: %s", ls_output.c_str());

      std::lock_guard<std::mutex> lock(*mtx_ptr);
      // TODO: analyze dns issues
      state_ptr->connect = (err1 == 0 || err2 == 0);
      SampleModel sm{};
      sm.created_at = monic::get_current_epoch();
      sm.connection_status = state_ptr->connect;
      sm.synced = false;
      sm.signal = ls_output;
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
  std::signal(SIGTERM, monic_signal_handler);

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

  std::thread task3(monic_kmsg_task, shared_state_ptr, &g_shutdown_requested,
                    &mtx);
  task1.join();
  task2.join();
  task3.join();

  log_info("program stopped\n");

  return 0;
}