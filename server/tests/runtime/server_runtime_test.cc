#include <array>
#include <cstdint>
#include <vector>

#include "asio/connect.hpp"
#include "asio/io_context.hpp"
#include "asio/ip/tcp.hpp"
#include "asio/read.hpp"
#include "asio/write.hpp"
#include "gtest/gtest.h"
#include "server/config/config_manager.h"
#include "server/runtime/server_runtime.h"
#include "shared/protocol/auth_messages.h"
#include "shared/protocol/runtime_messages.h"
#include "shared/protocol/wire_codec.h"

namespace server {
namespace {

GameConfig MakeValidConfig() {
  GameConfig config;
  config.item_templates.push_back(ItemTemplate{.id = 5001});
  config.skill_templates.push_back(SkillTemplate{.id = 1001, .range = 3});
  config.monster_templates.push_back(MonsterTemplate{
      .id = 2001,
      .drop_item_id = 5001,
      .move_speed = 1,
      .skill_id = 1001,
  });
  config.monster_spawns.push_back(MonsterSpawn{.monster_template_id = 2001});
  return config;
}

shared::Envelope ReadEnvelope(asio::ip::tcp::socket* socket) {
  EXPECT_NE(socket, nullptr);
  if (socket == nullptr) {
    return {};
  }
  std::array<std::uint8_t, 6> header{};
  asio::read(*socket, asio::buffer(header));
  const std::optional<shared::Envelope> header_envelope =
      shared::DecodeEnvelope(header);
  EXPECT_FALSE(header_envelope.has_value());

  const std::uint32_t payload_size =
      static_cast<std::uint32_t>(header[2]) |
      (static_cast<std::uint32_t>(header[3]) << 8U) |
      (static_cast<std::uint32_t>(header[4]) << 16U) |
      (static_cast<std::uint32_t>(header[5]) << 24U);
  std::vector<std::uint8_t> bytes(header.begin(), header.end());
  std::vector<std::uint8_t> payload(payload_size);
  if (payload_size != 0U) {
    asio::read(*socket, asio::buffer(payload));
    bytes.insert(bytes.end(), payload.begin(), payload.end());
  }
  const std::optional<shared::Envelope> envelope = shared::DecodeEnvelope(bytes);
  EXPECT_TRUE(envelope.has_value());
  return *envelope;
}

TEST(ServerRuntimeTest, TcpLoginAndEnterSceneReturnBootstrapAndSnapshot) {
  ConfigManager config_manager;
  config_manager.Load(MakeValidConfig());
  ServerRuntime runtime(config_manager, ServerRuntime::Options{
                                            .tcp_port = 0,
                                            .udp_port = 5001,
                                        });
  ASSERT_TRUE(runtime.Start());

  asio::io_context client_io;
  asio::ip::tcp::socket socket(client_io);
  asio::connect(socket, asio::ip::tcp::resolver(client_io).resolve(
                           "127.0.0.1", std::to_string(runtime.tcp_port())));

  const std::vector<std::uint8_t> login_request = shared::EncodeEnvelope(
      shared::LoginRequest::kMessageId,
      shared::EncodeMessage(shared::LoginRequest{"hero", "pw"}));
  asio::write(socket, asio::buffer(login_request));

  const shared::Envelope login_envelope = ReadEnvelope(&socket);
  ASSERT_EQ(login_envelope.message_id, shared::LoginResponse::kMessageId);
  const std::optional<shared::LoginResponse> login =
      shared::DecodeMessage<shared::LoginResponse>(login_envelope.payload);
  ASSERT_TRUE(login.has_value());
  EXPECT_EQ(login->error_code, shared::ErrorCode::kOk);

  const std::vector<std::uint8_t> enter_scene_request = shared::EncodeEnvelope(
      shared::EnterSceneRequest::kMessageId,
      shared::EncodeMessage(shared::EnterSceneRequest{login->player_id, 1}));
  asio::write(socket, asio::buffer(enter_scene_request));

  const shared::Envelope bootstrap_envelope = ReadEnvelope(&socket);
  ASSERT_EQ(bootstrap_envelope.message_id,
            shared::SceneChannelBootstrap::kMessageId);
  const std::optional<shared::SceneChannelBootstrap> bootstrap =
      shared::DecodeMessage<shared::SceneChannelBootstrap>(
          bootstrap_envelope.payload);
  ASSERT_TRUE(bootstrap.has_value());
  EXPECT_EQ(bootstrap->player_id, login->player_id);
  EXPECT_EQ(bootstrap->scene_id, 1U);
  EXPECT_EQ(bootstrap->udp_port, 5001U);

  const shared::Envelope snapshot_envelope = ReadEnvelope(&socket);
  ASSERT_EQ(snapshot_envelope.message_id,
            shared::EnterSceneSnapshot::kMessageId);
  const std::optional<shared::EnterSceneSnapshot> snapshot =
      shared::DecodeMessage<shared::EnterSceneSnapshot>(
          snapshot_envelope.payload);
  ASSERT_TRUE(snapshot.has_value());
  EXPECT_EQ(snapshot->player_id, login->player_id);
  EXPECT_EQ(snapshot->scene_id, 1U);
  EXPECT_FALSE(snapshot->visible_entities.empty());

  runtime.Stop();
}

}  // namespace
}  // namespace server
