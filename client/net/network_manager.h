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
#include "asio/ip/udp.hpp"
#include "asio/steady_timer.hpp"
#include "client/protocol/client_message.h"
#include "ikcp.h"  // NOLINT(build/include_subdir)

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
  void ConfigureSceneChannel(const shared::SceneChannelBootstrap& bootstrap);
  void SendSceneHello();
  void ReadHeader();
  void ReadPayload(std::uint32_t payload_size);
  void HandleEnvelopePayload(const std::vector<std::uint8_t>& bytes);
  void FlushWrites();
  void FlushSceneOutbound();
  void StartUdpReceive();
  void StartKcpTick();
  void SendSceneOutbound(const OutboundMessage& message);
  static int KcpOutput(const char* buffer, int length, ikcpcb* kcp, void* user);

  NetworkConfig config_;
  asio::io_context io_context_;
  asio::ip::tcp::resolver resolver_;
  asio::ip::tcp::socket tcp_socket_;
  asio::ip::udp::socket udp_socket_;
  asio::steady_timer kcp_tick_timer_;
  std::unique_ptr<WorkGuard> work_guard_;
  mutable std::mutex state_mutex_;
  std::mutex inbound_mutex_;
  std::mutex outbound_mutex_;
  std::thread io_thread_;
  ConnectionState connection_state_ = ConnectionState::kStopped;
  bool tcp_connected_ = false;
  bool scene_channel_ready_ = false;
  bool write_in_progress_ = false;
  std::uint32_t kcp_conv_ = 0;
  ikcpcb* kcp_ = nullptr;
  std::vector<Message> inbound_messages_;
  std::deque<std::vector<std::uint8_t>> pending_writes_;
  std::deque<OutboundMessage> pending_scene_outbound_;
  std::array<std::uint8_t, 6> header_buffer_{};
  std::array<std::uint8_t, 2048> udp_buffer_{};
  std::vector<std::uint8_t> payload_buffer_;
  asio::ip::udp::endpoint udp_server_endpoint_;
  asio::ip::udp::endpoint udp_sender_endpoint_;
  std::string scene_session_token_;
};

}  // namespace client

#endif  // CLIENT_NET_NETWORK_MANAGER_H_
