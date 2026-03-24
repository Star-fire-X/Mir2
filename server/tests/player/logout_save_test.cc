#include "gtest/gtest.h"
#include "server/db/save_service.h"
#include "server/ecs/components.h"
#include "server/net/session.h"
#include "server/player/player_manager.h"
#include "server/protocol/protocol_dispatcher.h"
#include "server/scene/scene_manager.h"

namespace server {
namespace {

TEST(LogoutSaveTest, LogoutQueuesSaveSnapshotFromLiveSceneState) {
  PlayerManager player_manager;
  SceneManager scene_manager;
  SaveService save_service;
  ProtocolDispatcher dispatcher(&player_manager, &scene_manager);
  Session session(1);

  const shared::LoginResponse login = dispatcher.HandleLogin(
      &session, shared::LoginRequest{"hero_account", "password"});
  const std::optional<shared::EnterSceneSnapshot> enter_scene_snapshot =
      dispatcher.HandleEnterScene(
          &session, shared::EnterSceneRequest{login.player_id, 1});
  ASSERT_TRUE(enter_scene_snapshot.has_value());

  Scene* scene = scene_manager.Find(1);
  ASSERT_NE(scene, nullptr);
  const std::optional<entt::entity> entity =
      scene->Find(enter_scene_snapshot->controlled_entity_id);
  ASSERT_TRUE(entity.has_value());
  scene->registry().get<ecs::PositionComponent>(*entity).position = {8.0F,
                                                                     6.0F};

  EXPECT_TRUE(dispatcher.HandleLogout(&session, &save_service));
  EXPECT_EQ(session.state(), SessionState::kDisconnected);
  EXPECT_TRUE(save_service.HasQueuedSnapshot());
  EXPECT_EQ(save_service.queued_snapshot().data.identity.player_id,
            login.player_id);
  EXPECT_EQ(save_service.queued_snapshot().data.last_scene_snapshot.scene_id,
            1u);
  EXPECT_FLOAT_EQ(
      save_service.queued_snapshot().data.last_scene_snapshot.position.x, 8.0F);
  EXPECT_FLOAT_EQ(
      save_service.queued_snapshot().data.last_scene_snapshot.position.y, 6.0F);
  EXPECT_FALSE(
      scene->Find(enter_scene_snapshot->controlled_entity_id).has_value());

  const Player* player = player_manager.Find(login.player_id);
  ASSERT_NE(player, nullptr);
  EXPECT_EQ(player->session(), nullptr);
  EXPECT_FALSE(player->controlled_entity_id().has_value());
}

TEST(LogoutSaveTest, DisconnectFreezesPlayerOperations) {
  PlayerManager player_manager;
  SceneManager scene_manager;
  SaveService save_service;
  ProtocolDispatcher dispatcher(&player_manager, &scene_manager);
  Session session(1);

  const shared::LoginResponse login = dispatcher.HandleLogin(
      &session, shared::LoginRequest{"hero_account", "password"});
  const std::optional<shared::EnterSceneSnapshot> enter_scene_snapshot =
      dispatcher.HandleEnterScene(
          &session, shared::EnterSceneRequest{login.player_id, 1});
  ASSERT_TRUE(enter_scene_snapshot.has_value());

  EXPECT_TRUE(dispatcher.HandleDisconnect(&session, &save_service));
  EXPECT_EQ(session.state(), SessionState::kDisconnected);

  const Player* player = player_manager.Find(login.player_id);
  ASSERT_NE(player, nullptr);
  EXPECT_TRUE(player->operations_frozen());
  EXPECT_EQ(player->session(), nullptr);
}

}  // namespace
}  // namespace server
