#include <chrono>
#include <optional>
#include <thread>  // NOLINT(build/c++11)
#include <variant>
#include <vector>

#include "client/net/network_manager.h"
#include "gtest/gtest.h"
#include "server/config/config_manager.h"
#include "server/runtime/server_runtime.h"

namespace client {
namespace {

server::GameConfig MakeValidConfig() {
  server::GameConfig config;
  config.item_templates.push_back(server::ItemTemplate{.id = 5001});
  config.skill_templates.push_back(server::SkillTemplate{.id = 1001, .range = 3});
  config.monster_templates.push_back(server::MonsterTemplate{
      .id = 2001,
      .drop_item_id = 5001,
      .move_speed = 1,
      .skill_id = 1001,
  });
  config.monster_spawns.push_back(server::MonsterSpawn{.monster_template_id = 2001});
  return config;
}

std::vector<protocol::ClientMessage> WaitForInbound(NetworkManager* network_manager,
                                                   std::size_t expected_count) {
  EXPECT_NE(network_manager, nullptr);
  std::vector<protocol::ClientMessage> inbound_messages;
  for (int attempt = 0; attempt < 20; ++attempt) {
    std::vector<protocol::ClientMessage> inbound =
        network_manager->DrainInbound();
    inbound_messages.insert(inbound_messages.end(), inbound.begin(), inbound.end());
    if (inbound_messages.size() >= expected_count) {
      return inbound_messages;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  return inbound_messages;
}

TEST(NetworkRuntimeTest, QueueOutboundDeliversTcpLoginAndEnterSceneResponses) {
  server::ConfigManager config_manager;
  config_manager.Load(MakeValidConfig());
  server::ServerRuntime runtime(config_manager, server::ServerRuntime::Options{
                                                    .tcp_port = 0,
                                                    .udp_port = 5001,
                                                });
  ASSERT_TRUE(runtime.Start());

  NetworkManager network_manager(NetworkConfig{
      .host = "127.0.0.1",
      .tcp_port = runtime.tcp_port(),
  });
  network_manager.Start();
  network_manager.QueueOutbound(protocol::OutboundMessage{
      shared::LoginRequest{"hero", "pw"}});

  const std::vector<protocol::ClientMessage> login_messages =
      WaitForInbound(&network_manager, 1U);
  ASSERT_EQ(login_messages.size(), 1U);
  ASSERT_TRUE(
      std::holds_alternative<protocol::LoginResponseMessage>(login_messages[0]));
  const shared::LoginResponse login =
      std::get<protocol::LoginResponseMessage>(login_messages[0]).response;
  EXPECT_EQ(login.error_code, shared::ErrorCode::kOk);

  network_manager.QueueOutbound(protocol::OutboundMessage{
      shared::EnterSceneRequest{login.player_id, 1}});
  const std::vector<protocol::ClientMessage> enter_messages =
      WaitForInbound(&network_manager, 2U);
  ASSERT_EQ(enter_messages.size(), 2U);
  ASSERT_TRUE(std::holds_alternative<protocol::SceneChannelBootstrapMessage>(
      enter_messages[0]));
  ASSERT_TRUE(std::holds_alternative<protocol::EnterSceneSnapshotMessage>(
      enter_messages[1]));

  const shared::SceneChannelBootstrap bootstrap =
      std::get<protocol::SceneChannelBootstrapMessage>(enter_messages[0])
          .bootstrap;
  const shared::EnterSceneSnapshot snapshot =
      std::get<protocol::EnterSceneSnapshotMessage>(enter_messages[1]).snapshot;
  EXPECT_EQ(bootstrap.player_id, login.player_id);
  EXPECT_EQ(snapshot.player_id, login.player_id);
  EXPECT_EQ(snapshot.scene_id, 1U);

  network_manager.Stop();
  runtime.Stop();
}

}  // namespace
}  // namespace client
