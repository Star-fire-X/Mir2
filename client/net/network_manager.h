#ifndef CLIENT_NET_NETWORK_MANAGER_H_
#define CLIENT_NET_NETWORK_MANAGER_H_

#include <array>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>  // NOLINT(build/c++11)
#include <string>
#include <thread>  // NOLINT(build/c++11)
#include <vector>

#include "asio/executor_work_guard.hpp"
#include "asio/io_context.hpp"
#include "asio/ip/tcp.hpp"
#include "client/protocol/client_message.h"

namespace client {

struct NetworkConfig {
  std::string host = "127.0.0.1";
  std::uint16_t tcp_port = 5000;
};

enum class ConnectionState : std::uint8_t {
  kStopped = 0,
  kRunning = 1,
};

class NetworkManager {
 public:
  using Message = protocol::ClientMessage;
  using OutboundMessage = protocol::OutboundMessage;

  explicit NetworkManager(NetworkConfig config = {});
  ~NetworkManager();

  void Start();
  void Stop();
  void QueueOutbound(OutboundMessage message);
  void Enqueue(Message message);
  [[nodiscard]] std::vector<Message> DrainInbound();
  [[nodiscard]] std::vector<Message> Drain();
  [[nodiscard]] ConnectionState connection_state() const;

 private:
  using WorkGuard = asio::executor_work_guard<asio::io_context::executor_type>;

  void ConnectTcp();
  void ReadHeader();
  void ReadPayload(std::uint32_t payload_size);
  void HandleEnvelopePayload(const std::vector<std::uint8_t>& bytes);
  void FlushWrites();

  NetworkConfig config_;
  asio::io_context io_context_;
  asio::ip::tcp::resolver resolver_;
  asio::ip::tcp::socket tcp_socket_;
  std::unique_ptr<WorkGuard> work_guard_;
  mutable std::mutex state_mutex_;
  std::mutex inbound_mutex_;
  std::mutex outbound_mutex_;
  std::thread io_thread_;
  ConnectionState connection_state_ = ConnectionState::kStopped;
  bool tcp_connected_ = false;
  bool write_in_progress_ = false;
  std::vector<Message> inbound_messages_;
  std::deque<std::vector<std::uint8_t>> pending_writes_;
  std::array<std::uint8_t, 6> header_buffer_{};
  std::vector<std::uint8_t> payload_buffer_;
};

}  // namespace client

#endif  // CLIENT_NET_NETWORK_MANAGER_H_
