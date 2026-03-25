#include "server/runtime/server_runtime.h"

#include <array>
#include <chrono>
#include <optional>
#include <string>
#include <system_error>
#include <type_traits>
#include <utility>
#include <vector>

#include "asio/buffer.hpp"
#include "asio/ip/address.hpp"
#include "asio/post.hpp"
#include "asio/read.hpp"
#include "asio/write.hpp"
#include "ikcp.h"
#include "shared/protocol/runtime_messages.h"
#include "shared/protocol/wire_codec.h"

namespace server {
namespace {

constexpr std::size_t kEnvelopeHeaderSize =
    sizeof(std::uint16_t) + sizeof(std::uint32_t);
constexpr std::size_t kMaxUdpPacketSize = 2048;

std::uint32_t DecodePayloadSize(const std::array<std::uint8_t, kEnvelopeHeaderSize>&
                                    header) {
  return static_cast<std::uint32_t>(header[2]) |
         (static_cast<std::uint32_t>(header[3]) << 8U) |
         (static_cast<std::uint32_t>(header[4]) << 16U) |
         (static_cast<std::uint32_t>(header[5]) << 24U);
}

std::uint32_t DecodeConv(std::span<const std::uint8_t> bytes) {
  if (bytes.size() < sizeof(std::uint32_t)) {
    return 0;
  }

  return static_cast<std::uint32_t>(bytes[0]) |
         (static_cast<std::uint32_t>(bytes[1]) << 8U) |
         (static_cast<std::uint32_t>(bytes[2]) << 16U) |
         (static_cast<std::uint32_t>(bytes[3]) << 24U);
}

std::uint32_t NowMs() {
  return static_cast<std::uint32_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::steady_clock::now().time_since_epoch())
          .count());
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

class ServerRuntime::KcpSceneChannel
    : public std::enable_shared_from_this<ServerRuntime::KcpSceneChannel> {
 public:
  KcpSceneChannel(std::uint32_t conv, ServerRuntime* runtime,
                  std::shared_ptr<ClientConnection> connection)
      : conv_(conv), runtime_(runtime), connection_(std::move(connection)) {
    kcp_ = ikcp_create(conv_, this);
    kcp_->output = &KcpSceneChannel::Output;
    ikcp_wndsize(kcp_, 128, 128);
    ikcp_nodelay(kcp_, 1, 10, 2, 1);
  }

  ~KcpSceneChannel() {
    if (kcp_ != nullptr) {
      ikcp_release(kcp_);
    }
  }

  Session* session() { return connection_->session(); }

  float AdvanceSceneTime() {
    now_seconds_ += 1.0F;
    return now_seconds_;
  }

  void SetRemoteEndpoint(const asio::ip::udp::endpoint& remote_endpoint) {
    remote_endpoint_ = remote_endpoint;
  }

  void Input(std::span<const std::uint8_t> bytes) {
    if (kcp_ == nullptr ||
        ikcp_input(kcp_, reinterpret_cast<const char*>(bytes.data()),
                   static_cast<long>(bytes.size())) < 0) {
      return;
    }
    DrainReceivedMessages();
  }

  void SendEnvelope(shared::MessageId message_id,
                    std::vector<std::uint8_t> payload) {
    if (kcp_ == nullptr) {
      return;
    }

    const std::vector<std::uint8_t> bytes =
        shared::EncodeEnvelope(message_id, payload);
    if (ikcp_send(kcp_, reinterpret_cast<const char*>(bytes.data()),
                  static_cast<int>(bytes.size())) < 0) {
      return;
    }
    ikcp_update(kcp_, NowMs());
  }

  void Tick() {
    if (kcp_ == nullptr) {
      return;
    }
    ikcp_update(kcp_, NowMs());
    DrainReceivedMessages();
  }

 private:
  static int Output(const char* buffer, int length, ikcpcb* /*kcp*/,
                    void* user) {
    return static_cast<KcpSceneChannel*>(user)->OutputImpl(buffer, length);
  }

  int OutputImpl(const char* buffer, int length) {
    if (!remote_endpoint_.has_value()) {
      return 0;
    }

    auto packet = std::make_shared<std::vector<std::uint8_t>>(
        buffer, buffer + static_cast<std::ptrdiff_t>(length));
    runtime_->udp_socket_.async_send_to(
        asio::buffer(*packet), *remote_endpoint_,
        [packet](const std::error_code& /*error*/, std::size_t /*written*/) {});
    return 0;
  }

  void DrainReceivedMessages() {
    std::array<char, kMaxUdpPacketSize> buffer{};
    while (true) {
      const int received = ikcp_recv(kcp_, buffer.data(),
                                     static_cast<int>(buffer.size()));
      if (received < 0) {
        return;
      }

      std::vector<std::uint8_t> bytes(
          buffer.begin(),
          buffer.begin() + static_cast<std::ptrdiff_t>(received));
      const std::optional<shared::Envelope> envelope =
          shared::DecodeEnvelope(bytes);
      if (!envelope.has_value()) {
        continue;
      }

      switch (envelope->message_id) {
        case shared::MoveRequest::kMessageId: {
          const auto move_request =
              shared::DecodeMessage<shared::MoveRequest>(envelope->payload);
          if (move_request.has_value()) {
            runtime_->HandleMove(shared_from_this(), *move_request);
          }
          break;
        }
        case shared::CastSkillRequest::kMessageId: {
          const auto cast_skill_request =
              shared::DecodeMessage<shared::CastSkillRequest>(envelope->payload);
          if (cast_skill_request.has_value()) {
            runtime_->HandleCastSkill(shared_from_this(), *cast_skill_request);
          }
          break;
        }
        case shared::PickupRequest::kMessageId: {
          const auto pickup_request =
              shared::DecodeMessage<shared::PickupRequest>(envelope->payload);
          if (pickup_request.has_value()) {
            runtime_->HandlePickup(shared_from_this(), *pickup_request);
          }
          break;
        }
        default:
          break;
      }
    }
  }

  std::uint32_t conv_ = 0;
  ServerRuntime* runtime_ = nullptr;
  std::shared_ptr<ClientConnection> connection_;
  ikcpcb* kcp_ = nullptr;
  std::optional<asio::ip::udp::endpoint> remote_endpoint_;
  float now_seconds_ = 0.0F;
};

ServerRuntime::ServerRuntime(const ConfigManager& config_manager)
    : ServerRuntime(config_manager, Options{}) {}

ServerRuntime::ServerRuntime(const ConfigManager& config_manager, Options options)
    : options_(std::move(options)),
      server_app_(config_manager),
      acceptor_(io_context_),
      udp_socket_(io_context_),
      kcp_tick_timer_(io_context_) {}

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

  const asio::ip::tcp::endpoint tcp_endpoint(bind_address, options_.tcp_port);
  acceptor_.open(tcp_endpoint.protocol(), error);
  if (error) {
    return false;
  }
  acceptor_.set_option(asio::ip::tcp::acceptor::reuse_address(true), error);
  if (error) {
    return false;
  }
  acceptor_.bind(tcp_endpoint, error);
  if (error) {
    return false;
  }
  acceptor_.listen(asio::socket_base::max_listen_connections, error);
  if (error) {
    return false;
  }
  options_.tcp_port = acceptor_.local_endpoint().port();

  const asio::ip::udp::endpoint udp_endpoint(bind_address, options_.udp_port);
  udp_socket_.open(udp_endpoint.protocol(), error);
  if (error) {
    return false;
  }
  udp_socket_.bind(udp_endpoint, error);
  if (error) {
    return false;
  }
  options_.udp_port = udp_socket_.local_endpoint().port();

  io_context_.restart();
  work_guard_ = std::make_unique<WorkGuard>(asio::make_work_guard(io_context_));
  running_.store(true);
  StartAccept();
  StartUdpReceive();
  StartKcpTick();
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
  if (logic_thread_.joinable()) {
    logic_thread_.join();
  }

  std::scoped_lock lock(channel_mutex_);
  scene_channels_.clear();
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

void ServerRuntime::StartUdpReceive() {
  udp_socket_.async_receive_from(
      asio::buffer(udp_read_buffer_), udp_remote_endpoint_,
      [this](const std::error_code& error, std::size_t bytes_received) {
        if (!error && bytes_received >= sizeof(std::uint32_t)) {
          const std::uint32_t conv = DecodeConv(
              std::span<const std::uint8_t>(udp_read_buffer_.data(), bytes_received));
          std::shared_ptr<KcpSceneChannel> channel;
          {
            std::scoped_lock lock(channel_mutex_);
            const auto it = scene_channels_.find(conv);
            if (it != scene_channels_.end()) {
              channel = it->second;
            }
          }
          if (channel != nullptr) {
            channel->SetRemoteEndpoint(udp_remote_endpoint_);
            channel->Input(std::span<const std::uint8_t>(udp_read_buffer_.data(),
                                                         bytes_received));
          }
        }

        if (running()) {
          StartUdpReceive();
        }
      });
}

void ServerRuntime::StartKcpTick() {
  kcp_tick_timer_.expires_after(std::chrono::milliseconds(10));
  kcp_tick_timer_.async_wait([this](const std::error_code& error) {
    if (error || !running()) {
      return;
    }

    std::vector<std::shared_ptr<KcpSceneChannel>> channels;
    {
      std::scoped_lock lock(channel_mutex_);
      channels.reserve(scene_channels_.size());
      for (const auto& [_, channel] : scene_channels_) {
        channels.push_back(channel);
      }
    }
    for (const auto& channel : channels) {
      channel->Tick();
    }
    StartKcpTick();
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
    const std::uint32_t conv =
        static_cast<std::uint32_t>(connection->session()->session_id());
    const auto channel =
        std::make_shared<KcpSceneChannel>(conv, this, connection);
    {
      std::scoped_lock lock(channel_mutex_);
      scene_channels_[conv] = channel;
    }

    const shared::SceneChannelBootstrap bootstrap{
        .player_id = enter_scene_request.player_id,
        .scene_id = enter_scene_request.scene_id,
        .kcp_conv = conv,
        .udp_port = options_.udp_port,
        .session_token =
            "session-" + std::to_string(connection->session()->session_id()),
    };

    asio::post(io_context_, [connection, bootstrap, events]() {
      connection->WriteEnvelope(shared::SceneChannelBootstrap::kMessageId,
                                shared::EncodeMessage(bootstrap));
      for (const ServerApp::OutboundEvent& event : events) {
        if (const auto* snapshot =
                std::get_if<shared::EnterSceneSnapshot>(&event);
            snapshot != nullptr) {
          connection->WriteEnvelope(shared::EnterSceneSnapshot::kMessageId,
                                    shared::EncodeMessage(*snapshot));
        }
      }
    });
  });
}

void ServerRuntime::HandleMove(const std::shared_ptr<KcpSceneChannel>& channel,
                               const shared::MoveRequest& move_request) {
  EnqueueLogicTask([this, channel, move_request]() {
    const std::vector<ServerApp::OutboundEvent> events =
        server_app_.HandleMove(channel->session(), move_request, 1.0F);
    asio::post(io_context_,
               [this, channel, events]() { SendOutboundEvents(channel, events); });
  });
}

void ServerRuntime::HandleCastSkill(
    const std::shared_ptr<KcpSceneChannel>& channel,
    const shared::CastSkillRequest& cast_skill_request) {
  EnqueueLogicTask([this, channel, cast_skill_request]() {
    const ServerApp::CastSkillOutcome outcome = server_app_.ResolveCastSkill(
        channel->session(), cast_skill_request, channel->AdvanceSceneTime());
    asio::post(io_context_, [this, channel, outcome]() {
      channel->SendEnvelope(shared::CastSkillResult::kMessageId,
                            shared::EncodeMessage(outcome.result));
      SendOutboundEvents(channel, outcome.events);
    });
  });
}

void ServerRuntime::HandlePickup(
    const std::shared_ptr<KcpSceneChannel>& channel,
    const shared::PickupRequest& pickup_request) {
  EnqueueLogicTask([this, channel, pickup_request]() {
    const ServerApp::PickupOutcome outcome =
        server_app_.ResolvePickup(channel->session(), pickup_request);
    asio::post(io_context_, [this, channel, outcome]() {
      channel->SendEnvelope(shared::PickupResult::kMessageId,
                            shared::EncodeMessage(outcome.result));
      SendOutboundEvents(channel, outcome.events);
    });
  });
}

void ServerRuntime::SendOutboundEvents(
    const std::shared_ptr<KcpSceneChannel>& channel,
    const std::vector<ServerApp::OutboundEvent>& events) {
  for (const ServerApp::OutboundEvent& event : events) {
    std::visit(
        [channel](const auto& payload) {
          using Payload = std::decay_t<decltype(payload)>;
          if constexpr (std::is_same_v<Payload, shared::EnterSceneSnapshot>) {
            return;
          } else if constexpr (std::is_same_v<Payload, ServerApp::SelfStateEvent>) {
            channel->SendEnvelope(
                shared::SelfState::kMessageId,
                shared::EncodeMessage(
                    shared::SelfState{payload.entity_id, payload.position}));
          } else if constexpr (std::is_same_v<Payload, ServerApp::AoiEnterEvent>) {
            channel->SendEnvelope(
                shared::AoiEnter::kMessageId,
                shared::EncodeMessage(shared::AoiEnter{payload.entity}));
          } else if constexpr (std::is_same_v<Payload, ServerApp::AoiLeaveEvent>) {
            channel->SendEnvelope(
                shared::AoiLeave::kMessageId,
                shared::EncodeMessage(shared::AoiLeave{payload.entity_id}));
          } else if constexpr (std::is_same_v<Payload, shared::InventoryDelta>) {
            channel->SendEnvelope(shared::InventoryDelta::kMessageId,
                                  shared::EncodeMessage(payload));
          }
        },
        event);
  }
}

}  // namespace server
