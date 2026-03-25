#include <algorithm>
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

TEST(NetworkRuntimeTest, KcpSceneChannelCarriesMoveCombatAndPickupFlow) {
  server::ConfigManager config_manager;
  config_manager.Load(MakeValidConfig());
  server::ServerRuntime runtime(config_manager, server::ServerRuntime::Options{
                                                    .tcp_port = 0,
                                                    .udp_port = 0,
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
  ASSERT_EQ(login.error_code, shared::ErrorCode::kOk);

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
  ASSERT_NE(bootstrap.kcp_conv, 0U);
  ASSERT_NE(bootstrap.udp_port, 0U);

  const auto monster_it =
      std::find_if(snapshot.visible_entities.begin(),
                   snapshot.visible_entities.end(),
                   [](const shared::VisibleEntitySnapshot& entity) {
                     return entity.kind == shared::VisibleEntityKind::kMonster;
                   });
  ASSERT_NE(monster_it, snapshot.visible_entities.end());

  network_manager.QueueOutbound(protocol::OutboundMessage{
      shared::MoveRequest{snapshot.controlled_entity_id,
                          shared::ScenePosition{6.0F, 4.0F}, 1, 1000}});
  const std::vector<protocol::ClientMessage> move_messages =
      WaitForInbound(&network_manager, 1U);
  ASSERT_EQ(move_messages.size(), 1U);
  ASSERT_TRUE(
      std::holds_alternative<protocol::SelfStateMessage>(move_messages[0]));
  const shared::SelfState self_state =
      std::get<protocol::SelfStateMessage>(move_messages[0]).state;
  EXPECT_EQ(self_state.entity_id, snapshot.controlled_entity_id);
  EXPECT_FLOAT_EQ(self_state.position.x, 6.0F);
  EXPECT_FLOAT_EQ(self_state.position.y, 4.0F);

  network_manager.QueueOutbound(protocol::OutboundMessage{
      shared::CastSkillRequest{
          snapshot.controlled_entity_id, monster_it->entity_id, 1001, 2}});
  const std::vector<protocol::ClientMessage> combat_messages =
      WaitForInbound(&network_manager, 3U);
  ASSERT_EQ(combat_messages.size(), 3U);
  ASSERT_TRUE(std::holds_alternative<protocol::CastSkillResultMessage>(
      combat_messages[0]));
  ASSERT_TRUE(
      std::holds_alternative<protocol::AoiLeaveMessage>(combat_messages[1]));
  ASSERT_TRUE(
      std::holds_alternative<protocol::AoiEnterMessage>(combat_messages[2]));

  const shared::CastSkillResult cast_result =
      std::get<protocol::CastSkillResultMessage>(combat_messages[0]).result;
  EXPECT_EQ(cast_result.error_code, shared::ErrorCode::kOk);
  EXPECT_EQ(cast_result.target_entity_id, monster_it->entity_id);

  const shared::AoiEnter drop_enter =
      std::get<protocol::AoiEnterMessage>(combat_messages[2]).event;
  EXPECT_EQ(drop_enter.entity.kind, shared::VisibleEntityKind::kDrop);

  network_manager.QueueOutbound(protocol::OutboundMessage{
      shared::PickupRequest{login.player_id, drop_enter.entity.entity_id, 3}});
  const std::vector<protocol::ClientMessage> pickup_messages =
      WaitForInbound(&network_manager, 3U);
  ASSERT_EQ(pickup_messages.size(), 3U);
  ASSERT_TRUE(std::holds_alternative<protocol::PickupResultMessage>(
      pickup_messages[0]));
  ASSERT_TRUE(
      std::holds_alternative<protocol::AoiLeaveMessage>(pickup_messages[1]));
  ASSERT_TRUE(std::holds_alternative<protocol::InventoryDeltaMessage>(
      pickup_messages[2]));

  const shared::PickupResult pickup_result =
      std::get<protocol::PickupResultMessage>(pickup_messages[0]).result;
  EXPECT_EQ(pickup_result.error_code, shared::ErrorCode::kOk);
  EXPECT_EQ(pickup_result.drop_entity_id, drop_enter.entity.entity_id);
  const shared::InventoryDelta inventory_delta =
      std::get<protocol::InventoryDeltaMessage>(pickup_messages[2]).delta;
  ASSERT_EQ(inventory_delta.slots.size(), 1U);
  EXPECT_EQ(inventory_delta.slots[0].item_template_id, 5001U);
  EXPECT_EQ(inventory_delta.slots[0].item_count, 1U);

  network_manager.Stop();
  runtime.Stop();
}

}  // namespace
}  // namespace client
