#include "client/net/network_manager.h"

#include <chrono>  // NOLINT(build/c++11)
#include <optional>
#include <string>
#include <system_error>  // NOLINT(build/c++11)
#include <type_traits>
#include <utility>
#include <vector>

#include "asio/buffer.hpp"
#include "asio/connect.hpp"
#include "asio/ip/address.hpp"
#include "asio/post.hpp"
#include "asio/read.hpp"
#include "asio/write.hpp"
#include "shared/protocol/wire_codec.h"

namespace client {
namespace {

using KcpSize = long;  // NOLINT(runtime/int)

constexpr std::size_t kEnvelopeHeaderSize =
    sizeof(std::uint16_t) + sizeof(std::uint32_t);

std::uint32_t DecodePayloadSize(
    const std::array<std::uint8_t, kEnvelopeHeaderSize>& header) {
  return static_cast<std::uint32_t>(header[2]) |
         (static_cast<std::uint32_t>(header[3]) << 8U) |
         (static_cast<std::uint32_t>(header[4]) << 16U) |
         (static_cast<std::uint32_t>(header[5]) << 24U);
}

std::uint32_t NowMs() {
  return static_cast<std::uint32_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::steady_clock::now().time_since_epoch())
          .count());
}

std::optional<protocol::ClientMessage> DecodeInboundMessage(
    const shared::Envelope& envelope) {
  switch (envelope.message_id) {
    case shared::LoginResponse::kMessageId: {
      const auto response =
          shared::DecodeMessage<shared::LoginResponse>(envelope.payload);
      if (!response.has_value()) {
        return std::nullopt;
      }
      return protocol::ClientMessage{protocol::LoginResponseMessage{*response}};
    }
    case shared::SceneChannelBootstrap::kMessageId: {
      const auto bootstrap =
          shared::DecodeMessage<shared::SceneChannelBootstrap>(
              envelope.payload);
      if (!bootstrap.has_value()) {
        return std::nullopt;
      }
      return protocol::ClientMessage{
          protocol::SceneChannelBootstrapMessage{*bootstrap}};
    }
    case shared::EnterSceneSnapshot::kMessageId: {
      const auto snapshot =
          shared::DecodeMessage<shared::EnterSceneSnapshot>(envelope.payload);
      if (!snapshot.has_value()) {
        return std::nullopt;
      }
      return protocol::ClientMessage{
          protocol::EnterSceneSnapshotMessage{*snapshot}};
    }
    case shared::SelfState::kMessageId: {
      const auto self_state =
          shared::DecodeMessage<shared::SelfState>(envelope.payload);
      if (!self_state.has_value()) {
        return std::nullopt;
      }
      return protocol::ClientMessage{protocol::SelfStateMessage{*self_state}};
    }
    case shared::AoiEnter::kMessageId: {
      const auto event =
          shared::DecodeMessage<shared::AoiEnter>(envelope.payload);
      if (!event.has_value()) {
        return std::nullopt;
      }
      return protocol::ClientMessage{protocol::AoiEnterMessage{*event}};
    }
    case shared::AoiLeave::kMessageId: {
      const auto event =
          shared::DecodeMessage<shared::AoiLeave>(envelope.payload);
      if (!event.has_value()) {
        return std::nullopt;
      }
      return protocol::ClientMessage{protocol::AoiLeaveMessage{*event}};
    }
    case shared::InventoryDelta::kMessageId: {
      const auto delta =
          shared::DecodeMessage<shared::InventoryDelta>(envelope.payload);
      if (!delta.has_value()) {
        return std::nullopt;
      }
      return protocol::ClientMessage{protocol::InventoryDeltaMessage{*delta}};
    }
    case shared::CastSkillResult::kMessageId: {
      const auto result =
          shared::DecodeMessage<shared::CastSkillResult>(envelope.payload);
      if (!result.has_value()) {
        return std::nullopt;
      }
      return protocol::ClientMessage{protocol::CastSkillResultMessage{*result}};
    }
    case shared::PickupResult::kMessageId: {
      const auto result =
          shared::DecodeMessage<shared::PickupResult>(envelope.payload);
      if (!result.has_value()) {
        return std::nullopt;
      }
      return protocol::ClientMessage{protocol::PickupResultMessage{*result}};
    }
    default:
      return std::nullopt;
  }
}

std::vector<std::uint8_t> EncodeEnvelopeForPayload(const auto& payload) {
  return shared::EncodeEnvelope(payload.kMessageId,
                                shared::EncodeMessage(payload));
}

}  // namespace

NetworkManager::NetworkManager(NetworkConfig config)
    : config_(std::move(config)),
      resolver_(io_context_),
      tcp_socket_(io_context_),
      udp_socket_(io_context_),
      kcp_tick_timer_(io_context_) {}

NetworkManager::~NetworkManager() { Stop(); }

void NetworkManager::Start() {
  std::scoped_lock state_lock{state_mutex_};
  if (connection_state_ == ConnectionState::kRunning) {
    return;
  }

  io_context_.restart();
  work_guard_ = std::make_unique<WorkGuard>(asio::make_work_guard(io_context_));
  tcp_connected_ = false;
  scene_channel_ready_ = false;
  write_in_progress_ = false;
  scene_session_token_.clear();
  pending_scene_outbound_.clear();
  if (kcp_ != nullptr) {
    ikcp_release(kcp_);
    kcp_ = nullptr;
  }
  io_thread_ = std::thread([this]() { io_context_.run(); });
  connection_state_ = ConnectionState::kRunning;
  asio::post(io_context_, [this]() { ConnectTcp(); });
}

void NetworkManager::Stop() {
  {
    std::scoped_lock state_lock{state_mutex_};
    if (connection_state_ == ConnectionState::kStopped) {
      return;
    }
    connection_state_ = ConnectionState::kStopped;
  }

  asio::post(io_context_, [this]() {
    std::error_code error;
    resolver_.cancel();
    tcp_socket_.close(error);
    udp_socket_.close(error);
    kcp_tick_timer_.cancel();
  });
  if (work_guard_ != nullptr) {
    work_guard_->reset();
    work_guard_.reset();
  }
  io_context_.stop();
  if (io_thread_.joinable()) {
    io_thread_.join();
  }
  if (kcp_ != nullptr) {
    ikcp_release(kcp_);
    kcp_ = nullptr;
  }
  tcp_connected_ = false;
  scene_channel_ready_ = false;
  write_in_progress_ = false;
  scene_session_token_.clear();
  pending_scene_outbound_.clear();
}

void NetworkManager::QueueOutbound(OutboundMessage message) {
  const bool is_scene_message = std::visit(
      [](const auto& payload) {
        using Payload = std::decay_t<decltype(payload)>;
        return std::is_same_v<Payload, shared::MoveRequest> ||
               std::is_same_v<Payload, shared::CastSkillRequest> ||
               std::is_same_v<Payload, shared::PickupRequest>;
      },
      message);

  if (is_scene_message) {
    asio::post(io_context_, [this, message = std::move(message)]() mutable {
      if (!scene_channel_ready_ || kcp_ == nullptr) {
        pending_scene_outbound_.push_back(std::move(message));
        return;
      }
      SendSceneOutbound(message);
    });
    return;
  }

  {
    std::scoped_lock lock{outbound_mutex_};
    pending_writes_.push_back(std::visit(
        [](const auto& payload) { return EncodeEnvelopeForPayload(payload); },
        message));
  }

  if (connection_state() == ConnectionState::kRunning) {
    asio::post(io_context_, [this]() { FlushWrites(); });
  }
}

void NetworkManager::Enqueue(Message message) {
  std::scoped_lock lock{inbound_mutex_};
  inbound_messages_.push_back(std::move(message));
}

std::vector<NetworkManager::Message> NetworkManager::DrainInbound() {
  std::scoped_lock lock{inbound_mutex_};
  std::vector<Message> drained;
  drained.swap(inbound_messages_);
  return drained;
}

std::vector<NetworkManager::Message> NetworkManager::Drain() {
  return DrainInbound();
}

ConnectionState NetworkManager::connection_state() const {
  std::scoped_lock lock{state_mutex_};
  return connection_state_;
}

void NetworkManager::ConnectTcp() {
  resolver_.async_resolve(
      config_.host, std::to_string(config_.tcp_port),
      [this](const std::error_code& error,
             asio::ip::tcp::resolver::results_type endpoints) {
        if (error) {
          return;
        }

        asio::async_connect(
            tcp_socket_, endpoints,
            [this](const std::error_code& connect_error,
                   const asio::ip::tcp::endpoint& /*endpoint*/) {
              if (connect_error) {
                return;
              }
              tcp_connected_ = true;
              ReadHeader();
              FlushWrites();
            });
      });
}

void NetworkManager::ConfigureSceneChannel(
    const shared::SceneChannelBootstrap& bootstrap) {
  const bool start_udp = !udp_socket_.is_open();
  const bool start_kcp_tick = kcp_ == nullptr;

  std::error_code error;
  const asio::ip::address address = asio::ip::make_address(config_.host, error);
  if (error) {
    return;
  }

  if (!udp_socket_.is_open()) {
    udp_socket_.open(asio::ip::udp::v4(), error);
    if (error) {
      return;
    }
    udp_socket_.bind(asio::ip::udp::endpoint(asio::ip::udp::v4(), 0), error);
    if (error) {
      return;
    }
  }

  if (kcp_ != nullptr) {
    ikcp_release(kcp_);
    kcp_ = nullptr;
  }
  kcp_conv_ = bootstrap.kcp_conv;
  udp_server_endpoint_ = asio::ip::udp::endpoint(address, bootstrap.udp_port);
  scene_session_token_ = bootstrap.session_token;
  kcp_ = ikcp_create(kcp_conv_, this);
  kcp_->output = &NetworkManager::KcpOutput;
  ikcp_wndsize(kcp_, 128, 128);
  ikcp_nodelay(kcp_, 1, 10, 2, 1);
  scene_channel_ready_ = false;

  if (start_udp) {
    StartUdpReceive();
  }
  if (start_kcp_tick) {
    StartKcpTick();
  }
  SendSceneHello();
}

void NetworkManager::SendSceneHello() {
  if (!udp_socket_.is_open() || udp_server_endpoint_.port() == 0 ||
      kcp_conv_ == 0 || scene_session_token_.empty()) {
    return;
  }

  auto bytes = std::make_shared<std::vector<std::uint8_t>>(
      shared::EncodeEnvelope(shared::SceneChannelHello::kMessageId,
                             shared::EncodeMessage(shared::SceneChannelHello{
                                 .kcp_conv = kcp_conv_,
                                 .session_token = scene_session_token_,
                             })));
  udp_socket_.async_send_to(
      asio::buffer(*bytes), udp_server_endpoint_,
      [bytes](const std::error_code& /*error*/, std::size_t /*written*/) {});
}

void NetworkManager::ReadHeader() {
  asio::async_read(tcp_socket_, asio::buffer(header_buffer_),
                   [this](const std::error_code& error, std::size_t /*read*/) {
                     if (error) {
                       return;
                     }
                     const std::uint32_t payload_size =
                         DecodePayloadSize(header_buffer_);
                     if (payload_size > shared::kMaxEnvelopePayloadSize) {
                       std::error_code close_error;
                       tcp_socket_.close(close_error);
                       tcp_connected_ = false;
                       return;
                     }
                     ReadPayload(payload_size);
                   });
}

void NetworkManager::ReadPayload(std::uint32_t payload_size) {
  if (payload_size > shared::kMaxEnvelopePayloadSize) {
    std::error_code close_error;
    tcp_socket_.close(close_error);
    tcp_connected_ = false;
    return;
  }
  payload_buffer_.assign(payload_size, 0);
  if (payload_size == 0U) {
    std::vector<std::uint8_t> bytes(header_buffer_.begin(),
                                    header_buffer_.end());
    HandleEnvelopePayload(bytes);
    ReadHeader();
    return;
  }

  asio::async_read(tcp_socket_, asio::buffer(payload_buffer_),
                   [this](const std::error_code& error, std::size_t /*read*/) {
                     if (error) {
                       return;
                     }
                     std::vector<std::uint8_t> bytes(header_buffer_.begin(),
                                                     header_buffer_.end());
                     bytes.insert(bytes.end(), payload_buffer_.begin(),
                                  payload_buffer_.end());
                     HandleEnvelopePayload(bytes);
                     ReadHeader();
                   });
}

void NetworkManager::HandleEnvelopePayload(
    const std::vector<std::uint8_t>& bytes) {
  const std::optional<shared::Envelope> envelope =
      shared::DecodeEnvelope(bytes);
  if (!envelope.has_value()) {
    return;
  }
  if (envelope->message_id == shared::SceneChannelHelloAck::kMessageId) {
    const std::optional<shared::SceneChannelHelloAck> ack =
        shared::DecodeMessage<shared::SceneChannelHelloAck>(envelope->payload);
    if (!ack.has_value() || ack->kcp_conv != kcp_conv_) {
      return;
    }
    if (ack->error_code == shared::ErrorCode::kOk) {
      scene_channel_ready_ = true;
      FlushSceneOutbound();
    }
    return;
  }
  const std::optional<protocol::ClientMessage> message =
      DecodeInboundMessage(*envelope);
  if (!message.has_value()) {
    return;
  }

  if (const auto* bootstrap =
          std::get_if<protocol::SceneChannelBootstrapMessage>(&*message);
      bootstrap != nullptr) {
    ConfigureSceneChannel(bootstrap->bootstrap);
  }

  std::scoped_lock lock{inbound_mutex_};
  inbound_messages_.push_back(*message);
}

void NetworkManager::FlushWrites() {
  if (!tcp_connected_ || write_in_progress_) {
    return;
  }

  std::shared_ptr<std::vector<std::uint8_t>> bytes;
  {
    std::scoped_lock lock{outbound_mutex_};
    if (pending_writes_.empty()) {
      return;
    }
    write_in_progress_ = true;
    bytes = std::make_shared<std::vector<std::uint8_t>>(
        std::move(pending_writes_.front()));
    pending_writes_.pop_front();
  }

  asio::async_write(
      tcp_socket_, asio::buffer(*bytes),
      [this, bytes](const std::error_code& error, std::size_t /*written*/) {
        write_in_progress_ = false;
        if (error) {
          tcp_connected_ = false;
          return;
        }
        FlushWrites();
      });
}

void NetworkManager::FlushSceneOutbound() {
  if (!scene_channel_ready_ || kcp_ == nullptr) {
    return;
  }

  while (!pending_scene_outbound_.empty()) {
    OutboundMessage message = std::move(pending_scene_outbound_.front());
    pending_scene_outbound_.pop_front();
    SendSceneOutbound(message);
  }
}

void NetworkManager::StartUdpReceive() {
  udp_socket_.async_receive_from(
      asio::buffer(udp_buffer_), udp_sender_endpoint_,
      [this](const std::error_code& error, std::size_t bytes_received) {
        if (!error && udp_sender_endpoint_ == udp_server_endpoint_) {
          std::vector<std::uint8_t> raw_bytes(
              udp_buffer_.begin(),
              udp_buffer_.begin() +
                  static_cast<std::ptrdiff_t>(bytes_received));
          const std::optional<shared::Envelope> raw_envelope =
              shared::DecodeEnvelope(raw_bytes);
          if (raw_envelope.has_value() &&
              raw_envelope->message_id ==
                  shared::SceneChannelHelloAck::kMessageId) {
            HandleEnvelopePayload(raw_bytes);
          } else if (scene_channel_ready_ && kcp_ != nullptr) {
            const KcpSize received_size = static_cast<KcpSize>(bytes_received);
            if (ikcp_input(kcp_,
                           reinterpret_cast<const char*>(udp_buffer_.data()),
                           received_size) >= 0) {
              std::array<char, 2048> buffer{};
              while (true) {
                const int received = ikcp_recv(kcp_, buffer.data(),
                                               static_cast<int>(buffer.size()));
                if (received < 0) {
                  break;
                }

                std::vector<std::uint8_t> bytes(
                    buffer.begin(),
                    buffer.begin() + static_cast<std::ptrdiff_t>(received));
                HandleEnvelopePayload(bytes);
              }
            }
          }
        }

        if (connection_state() == ConnectionState::kRunning &&
            udp_socket_.is_open()) {
          StartUdpReceive();
        }
      });
}

void NetworkManager::StartKcpTick() {
  kcp_tick_timer_.expires_after(std::chrono::milliseconds(10));
  kcp_tick_timer_.async_wait([this](const std::error_code& error) {
    if (error || kcp_ == nullptr) {
      return;
    }
    if (scene_channel_ready_) {
      ikcp_update(kcp_, NowMs());
    }
    StartKcpTick();
  });
}

void NetworkManager::SendSceneOutbound(const OutboundMessage& message) {
  if (!scene_channel_ready_ || kcp_ == nullptr) {
    return;
  }

  const std::vector<std::uint8_t> bytes = std::visit(
      [](const auto& payload) -> std::vector<std::uint8_t> {
        using Payload = std::decay_t<decltype(payload)>;
        if constexpr (std::is_same_v<Payload, shared::MoveRequest> ||
                      std::is_same_v<Payload, shared::CastSkillRequest> ||
                      std::is_same_v<Payload, shared::PickupRequest>) {
          return EncodeEnvelopeForPayload(payload);
        }
        return {};
      },
      message);
  if (bytes.empty()) {
    return;
  }

  if (ikcp_send(kcp_, reinterpret_cast<const char*>(bytes.data()),
                static_cast<int>(bytes.size())) < 0) {
    return;
  }
  ikcp_update(kcp_, NowMs());
}

int NetworkManager::KcpOutput(const char* buffer, int length, ikcpcb* /*kcp*/,
                              void* user) {
  auto* network_manager = static_cast<NetworkManager*>(user);
  if (!network_manager->udp_socket_.is_open()) {
    return 0;
  }

  auto packet = std::make_shared<std::vector<std::uint8_t>>(
      buffer, buffer + static_cast<std::ptrdiff_t>(length));
  network_manager->udp_socket_.async_send_to(
      asio::buffer(*packet), network_manager->udp_server_endpoint_,
      [packet](const std::error_code& /*error*/, std::size_t /*written*/) {});
  return 0;
}

}  // namespace client
