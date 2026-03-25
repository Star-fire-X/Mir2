#include <algorithm>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "client/app/game_app.h"
#include "client/protocol/client_message.h"
#include "gtest/gtest.h"
#include "server/app/server_app.h"
#include "server/config/config_manager.h"
#include "server/net/session.h"
#include "shared/protocol/auth_messages.h"
#include "shared/protocol/combat_messages.h"
#include "shared/protocol/scene_messages.h"

namespace server {
namespace {

GameConfig MakeClosedLoopConfig() {
  GameConfig config;
  config.item_templates.push_back(ItemTemplate{.id = 5001});
  config.skill_templates.push_back(SkillTemplate{.id = 1001, .range = 8});
  config.monster_templates.push_back(MonsterTemplate{
      .id = 2001,
      .drop_item_id = 5001,
      .move_speed = 2,
      .skill_id = 1001,
  });
  config.monster_spawns.push_back(MonsterSpawn{.monster_template_id = 2001});
  return config;
}

GameConfig MakeAoiBoundaryConfig() {
  GameConfig config = MakeClosedLoopConfig();
  config.monster_spawns.push_back(MonsterSpawn{.monster_template_id = 2001});
  return config;
}

std::optional<shared::EnterSceneSnapshot> FindSnapshot(
    const std::vector<ServerApp::OutboundEvent>& events) {
  const auto it = std::find_if(
      events.begin(), events.end(), [](const ServerApp::OutboundEvent& event) {
        return std::holds_alternative<shared::EnterSceneSnapshot>(event);
      });
  if (it != events.end()) {
    return std::get<shared::EnterSceneSnapshot>(*it);
  }
  return std::nullopt;
}

std::optional<shared::VisibleEntitySnapshot> FindAoiEnter(
    const std::vector<ServerApp::OutboundEvent>& events,
    shared::VisibleEntityKind kind) {
  const auto it = std::find_if(
      events.begin(), events.end(),
      [kind](const ServerApp::OutboundEvent& event) {
        return std::holds_alternative<ServerApp::AoiEnterEvent>(event) &&
               std::get<ServerApp::AoiEnterEvent>(event).entity.kind == kind;
      });
  if (it != events.end()) {
    return std::get<ServerApp::AoiEnterEvent>(*it).entity;
  }
  return std::nullopt;
}

std::optional<shared::InventoryDelta> FindInventoryDelta(
    const std::vector<ServerApp::OutboundEvent>& events) {
  const auto it = std::find_if(
      events.begin(), events.end(), [](const ServerApp::OutboundEvent& event) {
        return std::holds_alternative<shared::InventoryDelta>(event);
      });
  if (it != events.end()) {
    return std::get<shared::InventoryDelta>(*it);
  }
  return std::nullopt;
}

void EnqueueClientMessages(
    client::GameApp* game_app,
    const std::vector<ServerApp::OutboundEvent>& events) {
  ASSERT_NE(game_app, nullptr);

  for (const ServerApp::OutboundEvent& event : events) {
    std::visit(
        [game_app](const auto& payload) {
          using Payload = std::decay_t<decltype(payload)>;
          if constexpr (std::is_same_v<Payload, shared::EnterSceneSnapshot>) {
            game_app->network_manager().Enqueue(client::protocol::ClientMessage{
                client::protocol::EnterSceneSnapshotMessage{payload}});
            return;
          }
          if constexpr (std::is_same_v<Payload, ServerApp::SelfStateEvent>) {
            game_app->network_manager().Enqueue(client::protocol::ClientMessage{
                client::protocol::SelfStateMessage{
                    shared::SelfState{payload.entity_id, payload.position}}});
            return;
          }
          if constexpr (std::is_same_v<Payload, ServerApp::AoiEnterEvent>) {
            game_app->network_manager().Enqueue(client::protocol::ClientMessage{
                client::protocol::AoiEnterMessage{
                    shared::AoiEnter{payload.entity}}});
            return;
          }
          if constexpr (std::is_same_v<Payload, ServerApp::AoiLeaveEvent>) {
            game_app->network_manager().Enqueue(client::protocol::ClientMessage{
                client::protocol::AoiLeaveMessage{
                    shared::AoiLeave{payload.entity_id}}});
            return;
          }
          if constexpr (std::is_same_v<Payload, shared::InventoryDelta>) {
            game_app->network_manager().Enqueue(client::protocol::ClientMessage{
                client::protocol::InventoryDeltaMessage{payload}});
          }
        },
        event);
  }
}

TEST(ClosedLoopIntegrationTest, LoginEnterMoveCombatAndPickupStayInSync) {
  ConfigManager config_manager;
  config_manager.Load(MakeClosedLoopConfig());

  ServerApp server_app(config_manager);
  ASSERT_TRUE(server_app.Init());

  client::GameApp game_app;
  Session session(101);

  const shared::LoginResponse login =
      server_app.Login(&session, shared::LoginRequest{"hero", "pw"});
  ASSERT_EQ(login.error_code, shared::ErrorCode::kOk);

  const std::vector<ServerApp::OutboundEvent> enter_events =
      server_app.EnterScene(&session,
                            shared::EnterSceneRequest{login.player_id, 1});
  const std::optional<shared::EnterSceneSnapshot> enter_snapshot =
      FindSnapshot(enter_events);
  ASSERT_TRUE(enter_snapshot.has_value());

  EnqueueClientMessages(&game_app, enter_events);
  game_app.RunFrame();

  EXPECT_EQ(game_app.model_root().player_model().scene_id(), 1U);
  EXPECT_EQ(game_app.model_root().scene_state_model().scene_id(), 1U);
  EXPECT_EQ(game_app.model_root().player_model().controlled_entity_id(),
            enter_snapshot->controlled_entity_id);

  const client::Scene* client_scene =
      game_app.scene_manager().Find(enter_snapshot->scene_id);
  ASSERT_NE(client_scene, nullptr);
  EXPECT_EQ(client_scene->ViewCount(), 2U);

  const auto monster_it =
      std::find_if(enter_snapshot->visible_entities.begin(),
                   enter_snapshot->visible_entities.end(),
                   [](const shared::VisibleEntitySnapshot& entity) {
                     return entity.kind == shared::VisibleEntityKind::kMonster;
                   });
  ASSERT_NE(monster_it, enter_snapshot->visible_entities.end());

  const std::vector<ServerApp::OutboundEvent> move_events =
      server_app.HandleMove(&session,
                            shared::MoveRequest{
                                enter_snapshot->controlled_entity_id,
                                shared::ScenePosition{6.0F, 4.0F},
                                1,
                                1000,
                            },
                            1.0F);
  EnqueueClientMessages(&game_app, move_events);
  game_app.RunFrame();

  EXPECT_FLOAT_EQ(game_app.model_root().player_model().position().x, 6.0F);
  EXPECT_FLOAT_EQ(game_app.model_root().player_model().position().y, 4.0F);

  const std::vector<ServerApp::OutboundEvent> combat_events =
      server_app.HandleCastSkill(&session,
                                 shared::CastSkillRequest{
                                     enter_snapshot->controlled_entity_id,
                                     monster_it->entity_id,
                                     1001,
                                     2,
                                 },
                                 1.0F);
  const std::optional<shared::VisibleEntitySnapshot> drop_snapshot =
      FindAoiEnter(combat_events, shared::VisibleEntityKind::kDrop);
  ASSERT_TRUE(drop_snapshot.has_value());

  EnqueueClientMessages(&game_app, combat_events);
  game_app.RunFrame();

  EXPECT_EQ(client_scene->FindView(monster_it->entity_id), nullptr);
  ASSERT_NE(client_scene->FindView(drop_snapshot->entity_id), nullptr);

  const std::vector<ServerApp::OutboundEvent> pickup_events =
      server_app.HandlePickup(&session, shared::PickupRequest{
                                            login.player_id,
                                            drop_snapshot->entity_id,
                                            3,
                                        });
  const std::optional<shared::InventoryDelta> inventory_delta =
      FindInventoryDelta(pickup_events);
  ASSERT_TRUE(inventory_delta.has_value());

  EnqueueClientMessages(&game_app, pickup_events);
  game_app.RunFrame();

  EXPECT_EQ(client_scene->FindView(drop_snapshot->entity_id), nullptr);
  ASSERT_FALSE(
      game_app.ui_manager().inventory_panel().rendered_slots().empty());
  EXPECT_EQ(game_app.ui_manager()
                .inventory_panel()
                .rendered_slots()[0]
                .item_template_id,
            5001U);
  EXPECT_EQ(
      game_app.ui_manager().inventory_panel().rendered_slots()[0].item_count,
      1U);
  EXPECT_EQ(game_app.dev_panel().snapshot().scene_id, 1U);
  EXPECT_EQ(game_app.dev_panel().snapshot().entity_count, 1U);
  EXPECT_EQ(game_app.dev_panel().snapshot().recent_protocol_summaries.back(),
            "inventory_delta");
}

TEST(ClosedLoopIntegrationTest, MoveEnteringAoiKeepsClientSceneInSync) {
  ConfigManager config_manager;
  config_manager.Load(MakeAoiBoundaryConfig());

  ServerApp server_app(config_manager);
  ASSERT_TRUE(server_app.Init());

  client::GameApp game_app;
  Session session(201);

  const shared::LoginResponse login =
      server_app.Login(&session, shared::LoginRequest{"hero", "pw"});
  ASSERT_EQ(login.error_code, shared::ErrorCode::kOk);

  const std::vector<ServerApp::OutboundEvent> enter_events =
      server_app.EnterScene(&session,
                            shared::EnterSceneRequest{login.player_id, 1});
  const std::optional<shared::EnterSceneSnapshot> enter_snapshot =
      FindSnapshot(enter_events);
  ASSERT_TRUE(enter_snapshot.has_value());
  EXPECT_EQ(enter_snapshot->visible_entities.size(), 2U);

  EnqueueClientMessages(&game_app, enter_events);
  game_app.RunFrame();

  const client::Scene* client_scene =
      game_app.scene_manager().Find(enter_snapshot->scene_id);
  ASSERT_NE(client_scene, nullptr);
  EXPECT_EQ(client_scene->ViewCount(), 2U);

  const std::vector<ServerApp::OutboundEvent> move_events =
      server_app.HandleMove(&session,
                            shared::MoveRequest{
                                enter_snapshot->controlled_entity_id,
                                shared::ScenePosition{6.0F, 4.0F},
                                1,
                                1000,
                            },
                            1.0F);

  ASSERT_TRUE(std::any_of(
      move_events.begin(), move_events.end(),
      [](const ServerApp::OutboundEvent& event) {
        return std::holds_alternative<ServerApp::AoiEnterEvent>(event);
      }));

  EnqueueClientMessages(&game_app, move_events);
  game_app.RunFrame();

  EXPECT_EQ(client_scene->ViewCount(), 3U);
  EXPECT_EQ(game_app.model_root().scene_state_model().visible_entity_count(),
            3U);
}

}  // namespace
}  // namespace server
