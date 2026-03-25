#ifndef SERVER_RUNTIME_SERVER_RUNTIME_H_
#define SERVER_RUNTIME_SERVER_RUNTIME_H_

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>  // NOLINT(build/c++11)
#include <vector>

#include "asio/executor_work_guard.hpp"
#include "asio/io_context.hpp"
#include "asio/ip/tcp.hpp"
#include "server/app/server_app.h"

namespace server {

class ServerRuntime {
 public:
  struct Options {
    std::string bind_address = "127.0.0.1";
    std::uint16_t tcp_port = 5000;
    std::uint16_t udp_port = 5001;
  };

  explicit ServerRuntime(const ConfigManager& config_manager);
  ServerRuntime(const ConfigManager& config_manager, Options options);
  ~ServerRuntime();

  bool Start();
  void Stop();

  [[nodiscard]] bool running() const;
  [[nodiscard]] std::uint16_t tcp_port() const;
  [[nodiscard]] std::uint16_t udp_port() const;

 private:
  class ClientConnection;
  using WorkGuard = asio::executor_work_guard<asio::io_context::executor_type>;

  void StartAccept();
  void LogicLoop();
  void EnqueueLogicTask(std::function<void()> task);
  void HandleLogin(const std::shared_ptr<ClientConnection>& connection,
                   const shared::LoginRequest& login_request);
  void HandleEnterScene(
      const std::shared_ptr<ClientConnection>& connection,
      const shared::EnterSceneRequest& enter_scene_request);
  void SendOutboundEvents(
      const std::shared_ptr<ClientConnection>& connection,
      const std::vector<ServerApp::OutboundEvent>& events);

  Options options_;
  ServerApp server_app_;
  asio::io_context io_context_;
  std::unique_ptr<WorkGuard> work_guard_;
  asio::ip::tcp::acceptor acceptor_;
  std::thread io_thread_;
  std::thread logic_thread_;
  mutable std::mutex state_mutex_;
  std::mutex queue_mutex_;
  std::condition_variable queue_cv_;
  std::queue<std::function<void()>> logic_tasks_;
  std::atomic<bool> running_{false};
  std::atomic<std::uint64_t> next_session_id_{1};
};

}  // namespace server

#endif  // SERVER_RUNTIME_SERVER_RUNTIME_H_
