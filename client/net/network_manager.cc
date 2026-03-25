#include "client/net/network_manager.h"

#include <optional>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include "asio/buffer.hpp"
#include "asio/connect.hpp"
#include "asio/post.hpp"
#include "asio/read.hpp"
#include "asio/write.hpp"
#include "shared/protocol/runtime_messages.h"
#include "shared/protocol/wire_codec.h"

namespace client {
namespace {

constexpr std::size_t kEnvelopeHeaderSize =
    sizeof(std::uint16_t) + sizeof(std::uint32_t);

std::uint32_t DecodePayloadSize(
    const std::array<std::uint8_t, kEnvelopeHeaderSize>& header) {
  return static_cast<std::uint32_t>(header[2]) |
         (static_cast<std::uint32_t>(header[3]) << 8U) |
         (static_cast<std::uint32_t>(header[4]) << 16U) |
         (static_cast<std::uint32_t>(header[5]) << 24U);
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
          shared::DecodeMessage<shared::SceneChannelBootstrap>(envelope.payload);
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

std::vector<std::uint8_t> EncodeOutboundMessage(
    const protocol::OutboundMessage& message) {
  return std::visit(
      [](const auto& payload) {
        return shared::EncodeEnvelope(payload.kMessageId,
                                      shared::EncodeMessage(payload));
      },
      message);
}

}  // namespace

NetworkManager::NetworkManager(NetworkConfig config)
    : config_(std::move(config)), resolver_(io_context_), tcp_socket_(io_context_) {}

NetworkManager::~NetworkManager() { Stop(); }

void NetworkManager::Start() {
  std::scoped_lock state_lock{state_mutex_};
  if (connection_state_ == ConnectionState::kRunning) {
    return;
  }

  io_context_.restart();
  work_guard_ = std::make_unique<WorkGuard>(asio::make_work_guard(io_context_));
  tcp_connected_ = false;
  write_in_progress_ = false;
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
  });
  if (work_guard_ != nullptr) {
    work_guard_->reset();
    work_guard_.reset();
  }
  io_context_.stop();
  if (io_thread_.joinable()) {
    io_thread_.join();
  }
  tcp_connected_ = false;
  write_in_progress_ = false;
}

void NetworkManager::QueueOutbound(OutboundMessage message) {
  {
    std::scoped_lock lock{outbound_mutex_};
    pending_writes_.push_back(EncodeOutboundMessage(message));
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

void NetworkManager::ReadHeader() {
  asio::async_read(
      tcp_socket_, asio::buffer(header_buffer_),
      [this](const std::error_code& error, std::size_t /*read*/) {
        if (error) {
          return;
        }
        ReadPayload(DecodePayloadSize(header_buffer_));
      });
}

void NetworkManager::ReadPayload(std::uint32_t payload_size) {
  payload_buffer_.assign(payload_size, 0);
  if (payload_size == 0U) {
    std::vector<std::uint8_t> bytes(header_buffer_.begin(), header_buffer_.end());
    HandleEnvelopePayload(bytes);
    ReadHeader();
    return;
  }

  asio::async_read(
      tcp_socket_, asio::buffer(payload_buffer_),
      [this](const std::error_code& error, std::size_t /*read*/) {
        if (error) {
          return;
        }
        std::vector<std::uint8_t> bytes(header_buffer_.begin(), header_buffer_.end());
        bytes.insert(bytes.end(), payload_buffer_.begin(), payload_buffer_.end());
        HandleEnvelopePayload(bytes);
        ReadHeader();
      });
}

void NetworkManager::HandleEnvelopePayload(const std::vector<std::uint8_t>& bytes) {
  const std::optional<shared::Envelope> envelope = shared::DecodeEnvelope(bytes);
  if (!envelope.has_value()) {
    return;
  }
  const std::optional<protocol::ClientMessage> message =
      DecodeInboundMessage(*envelope);
  if (!message.has_value()) {
    return;
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

}  // namespace client
