#include "server/runtime/server_runtime.h"

#include <array>
#include <chrono>
#include <system_error>
#include <utility>
#include <vector>

#include "asio/buffer.hpp"
#include "asio/ip/address.hpp"
#include "asio/post.hpp"
#include "asio/read.hpp"
#include "asio/write.hpp"
#include "shared/protocol/runtime_messages.h"
#include "shared/protocol/wire_codec.h"

namespace server {
namespace {

constexpr std::size_t kEnvelopeHeaderSize =
    sizeof(std::uint16_t) + sizeof(std::uint32_t);

std::uint32_t DecodePayloadSize(const std::array<std::uint8_t, kEnvelopeHeaderSize>&
                                    header) {
  return static_cast<std::uint32_t>(header[2]) |
         (static_cast<std::uint32_t>(header[3]) << 8U) |
         (static_cast<std::uint32_t>(header[4]) << 16U) |
         (static_cast<std::uint32_t>(header[5]) << 24U);
}

}  // namespace

class ServerRuntime::ClientConnection
    : public std::enable_shared_from_this<ServerRuntime::ClientConnection> {
 public:
  ClientConnection(asio::ip::tcp::socket socket, std::uint64_t session_id,
                   ServerRuntime* runtime)
      : socket_(std::move(socket)), session_(session_id), runtime_(runtime) {}

  void Start() { ReadHeader(); }

  Session* session() { return &session_; }

  void WriteEnvelope(shared::MessageId message_id,
                     std::vector<std::uint8_t> payload) {
    auto bytes = std::make_shared<std::vector<std::uint8_t>>(
        shared::EncodeEnvelope(message_id, payload));
    auto self = shared_from_this();
    asio::async_write(
        socket_, asio::buffer(*bytes),
        [self, bytes](const std::error_code& error, std::size_t /*written*/) {
          if (error) {
            self->Close();
          }
        });
  }

 private:
  void ReadHeader() {
    auto self = shared_from_this();
    asio::async_read(
        socket_, asio::buffer(header_buffer_),
        [self](const std::error_code& error, std::size_t /*read*/) {
          if (error) {
            self->Close();
            return;
          }
          self->ReadPayload(DecodePayloadSize(self->header_buffer_));
        });
  }

  void ReadPayload(std::uint32_t payload_size) {
    payload_buffer_.assign(payload_size, 0);
    auto self = shared_from_this();
    if (payload_size == 0U) {
      HandleDecodedEnvelope();
      return;
    }

    asio::async_read(
        socket_, asio::buffer(payload_buffer_),
        [self](const std::error_code& error, std::size_t /*read*/) {
          if (error) {
            self->Close();
            return;
          }
          self->HandleDecodedEnvelope();
        });
  }

  void HandleDecodedEnvelope() {
    std::vector<std::uint8_t> bytes(header_buffer_.begin(), header_buffer_.end());
    bytes.insert(bytes.end(), payload_buffer_.begin(), payload_buffer_.end());
    const std::optional<shared::Envelope> envelope = shared::DecodeEnvelope(bytes);
    if (!envelope.has_value()) {
      Close();
      return;
    }

    switch (envelope->message_id) {
      case shared::LoginRequest::kMessageId: {
        const std::optional<shared::LoginRequest> login_request =
            shared::DecodeMessage<shared::LoginRequest>(envelope->payload);
        if (!login_request.has_value()) {
          Close();
          return;
        }
        runtime_->HandleLogin(shared_from_this(), *login_request);
        break;
      }
      case shared::EnterSceneRequest::kMessageId: {
        const std::optional<shared::EnterSceneRequest> enter_scene_request =
            shared::DecodeMessage<shared::EnterSceneRequest>(envelope->payload);
        if (!enter_scene_request.has_value()) {
          Close();
          return;
        }
        runtime_->HandleEnterScene(shared_from_this(), *enter_scene_request);
        break;
      }
      default:
        Close();
        return;
    }

    ReadHeader();
  }

  void Close() {
    std::error_code error;
    socket_.close(error);
    session_.Disconnect();
  }

  asio::ip::tcp::socket socket_;
  Session session_;
  ServerRuntime* runtime_ = nullptr;
  std::array<std::uint8_t, kEnvelopeHeaderSize> header_buffer_{};
  std::vector<std::uint8_t> payload_buffer_;
};

ServerRuntime::ServerRuntime(const ConfigManager& config_manager)
    : ServerRuntime(config_manager, Options{}) {}

ServerRuntime::ServerRuntime(const ConfigManager& config_manager, Options options)
    : options_(std::move(options)),
      server_app_(config_manager),
      acceptor_(io_context_) {}

ServerRuntime::~ServerRuntime() { Stop(); }

bool ServerRuntime::Start() {
  if (running()) {
    return true;
  }
  if (!server_app_.Init()) {
    return false;
  }

  std::error_code error;
  const asio::ip::address bind_address =
      asio::ip::make_address(options_.bind_address, error);
  if (error) {
    return false;
  }

  const asio::ip::tcp::endpoint endpoint(bind_address, options_.tcp_port);
  acceptor_.open(endpoint.protocol(), error);
  if (error) {
    return false;
  }
  acceptor_.set_option(asio::ip::tcp::acceptor::reuse_address(true), error);
  if (error) {
    return false;
  }
  acceptor_.bind(endpoint, error);
  if (error) {
    return false;
  }
  acceptor_.listen(asio::socket_base::max_listen_connections, error);
  if (error) {
    return false;
  }
  options_.tcp_port = acceptor_.local_endpoint().port();

  io_context_.restart();
  work_guard_ = std::make_unique<WorkGuard>(asio::make_work_guard(io_context_));
  running_.store(true);
  StartAccept();
  io_thread_ = std::thread([this]() { io_context_.run(); });
  logic_thread_ = std::thread([this]() { LogicLoop(); });
  return true;
}

void ServerRuntime::Stop() {
  const bool was_running = running_.exchange(false);
  if (!was_running) {
    return;
  }

  queue_cv_.notify_all();
  asio::post(io_context_, [this]() {
    std::error_code error;
    acceptor_.close(error);
  });
  if (work_guard_ != nullptr) {
    work_guard_->reset();
    work_guard_.reset();
  }
  io_context_.stop();
  if (io_thread_.joinable()) {
    io_thread_.join();
  }
  if (logic_thread_.joinable()) {
    logic_thread_.join();
  }
}

bool ServerRuntime::running() const { return running_.load(); }

std::uint16_t ServerRuntime::tcp_port() const { return options_.tcp_port; }

std::uint16_t ServerRuntime::udp_port() const { return options_.udp_port; }

void ServerRuntime::StartAccept() {
  acceptor_.async_accept(
      [this](const std::error_code& error, asio::ip::tcp::socket socket) {
        if (!error) {
          auto connection = std::make_shared<ClientConnection>(
              std::move(socket), next_session_id_.fetch_add(1), this);
          connection->Start();
        }
        if (running()) {
          StartAccept();
        }
      });
}

void ServerRuntime::LogicLoop() {
  while (running()) {
    std::queue<std::function<void()>> tasks;
    {
      std::unique_lock lock(queue_mutex_);
      queue_cv_.wait_for(lock, std::chrono::milliseconds(50), [this]() {
        return !logic_tasks_.empty() || !running();
      });
      tasks.swap(logic_tasks_);
    }

    while (!tasks.empty()) {
      tasks.front()();
      tasks.pop();
    }
  }
}

void ServerRuntime::EnqueueLogicTask(std::function<void()> task) {
  {
    std::scoped_lock lock(queue_mutex_);
    logic_tasks_.push(std::move(task));
  }
  queue_cv_.notify_one();
}

void ServerRuntime::HandleLogin(
    const std::shared_ptr<ClientConnection>& connection,
    const shared::LoginRequest& login_request) {
  EnqueueLogicTask([this, connection, login_request]() {
    const shared::LoginResponse response =
        server_app_.Login(connection->session(), login_request);
    asio::post(io_context_, [connection, response]() {
      connection->WriteEnvelope(shared::LoginResponse::kMessageId,
                                shared::EncodeMessage(response));
    });
  });
}

void ServerRuntime::HandleEnterScene(
    const std::shared_ptr<ClientConnection>& connection,
    const shared::EnterSceneRequest& enter_scene_request) {
  EnqueueLogicTask([this, connection, enter_scene_request]() {
    const std::vector<ServerApp::OutboundEvent> events =
        server_app_.EnterScene(connection->session(), enter_scene_request);
    const shared::SceneChannelBootstrap bootstrap{
        .player_id = enter_scene_request.player_id,
        .scene_id = enter_scene_request.scene_id,
        .kcp_conv = static_cast<std::uint32_t>(connection->session()->session_id()),
        .udp_port = options_.udp_port,
        .session_token =
            "session-" + std::to_string(connection->session()->session_id()),
    };

    asio::post(io_context_, [this, connection, bootstrap, events]() {
      connection->WriteEnvelope(shared::SceneChannelBootstrap::kMessageId,
                                shared::EncodeMessage(bootstrap));
      SendOutboundEvents(connection, events);
    });
  });
}

void ServerRuntime::SendOutboundEvents(
    const std::shared_ptr<ClientConnection>& connection,
    const std::vector<ServerApp::OutboundEvent>& events) {
  for (const ServerApp::OutboundEvent& event : events) {
    std::visit(
        [connection](const auto& payload) {
          using Payload = std::decay_t<decltype(payload)>;
          if constexpr (std::is_same_v<Payload, shared::EnterSceneSnapshot>) {
            connection->WriteEnvelope(shared::EnterSceneSnapshot::kMessageId,
                                      shared::EncodeMessage(payload));
          } else if constexpr (std::is_same_v<Payload, ServerApp::SelfStateEvent>) {
            connection->WriteEnvelope(
                shared::SelfState::kMessageId,
                shared::EncodeMessage(
                    shared::SelfState{payload.entity_id, payload.position}));
          } else if constexpr (std::is_same_v<Payload, ServerApp::AoiEnterEvent>) {
            connection->WriteEnvelope(
                shared::AoiEnter::kMessageId,
                shared::EncodeMessage(shared::AoiEnter{payload.entity}));
          } else if constexpr (std::is_same_v<Payload, ServerApp::AoiLeaveEvent>) {
            connection->WriteEnvelope(
                shared::AoiLeave::kMessageId,
                shared::EncodeMessage(shared::AoiLeave{payload.entity_id}));
          } else if constexpr (std::is_same_v<Payload, shared::InventoryDelta>) {
            connection->WriteEnvelope(shared::InventoryDelta::kMessageId,
                                      shared::EncodeMessage(payload));
          }
        },
        event);
  }
}

}  // namespace server
