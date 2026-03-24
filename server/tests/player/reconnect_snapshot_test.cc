#include <algorithm>

#include "gtest/gtest.h"
#include "server/db/save_service.h"
#include "server/ecs/components.h"
#include "server/entity/entity_factory.h"
#include "server/net/session.h"
#include "server/player/player_manager.h"
#include "server/protocol/protocol_dispatcher.h"
#include "server/scene/scene_manager.h"

namespace server {
namespace {

bool ContainsVisibleEntity(
    const std::vector<shared::VisibleEntitySnapshot>& visible_entities,
    shared::EntityId entity_id) {
  return std::find_if(
             visible_entities.begin(), visible_entities.end(),
             [entity_id](const shared::VisibleEntitySnapshot& snapshot) {
               return snapshot.entity_id == entity_id;
             }) != visible_entities.end();
}

TEST(ReconnectSnapshotTest, ReconnectReturnsFreshFullSceneSnapshot) {
  PlayerManager player_manager;
  SceneManager scene_manager;
  SaveService save_service;
  ProtocolDispatcher dispatcher(&player_manager, &scene_manager);
  Session first_session(1);

  const shared::LoginResponse login = dispatcher.HandleLogin(
      &first_session, shared::LoginRequest{"hero_account", "password"});
  const std::optional<shared::EnterSceneSnapshot> first_snapshot =
      dispatcher.HandleEnterScene(
          &first_session, shared::EnterSceneRequest{login.player_id, 1});
  ASSERT_TRUE(first_snapshot.has_value());

  Scene* scene = scene_manager.Find(1);
  ASSERT_NE(scene, nullptr);
  EntityFactory entity_factory(scene);
  constexpr shared::EntityId kMonsterId{900001};
  entity_factory.SpawnMonster(2001, kMonsterId,
                              shared::ScenePosition{7.0F, 4.0F});

  const std::optional<entt::entity> controlled_entity =
      scene->Find(first_snapshot->controlled_entity_id);
  ASSERT_TRUE(controlled_entity.has_value());
  scene->registry().get<ecs::PositionComponent>(*controlled_entity).position = {
      6.0F,
      4.0F,
  };

  const Player* player = player_manager.Find(login.player_id);
  ASSERT_NE(player, nullptr);

  EXPECT_TRUE(dispatcher.HandleDisconnect(&first_session, &save_service));

  Session second_session(2);
  const std::optional<shared::EnterSceneSnapshot> reconnect_snapshot =
      dispatcher.HandleReconnect(&second_session, login.player_id);

  ASSERT_TRUE(reconnect_snapshot.has_value());
  EXPECT_EQ(second_session.state(), SessionState::kInScene);
  EXPECT_EQ(reconnect_snapshot->player_id, login.player_id);
  EXPECT_EQ(reconnect_snapshot->scene_id, 1u);
  EXPECT_FLOAT_EQ(reconnect_snapshot->self_position.x, 6.0F);
  EXPECT_FLOAT_EQ(reconnect_snapshot->self_position.y, 4.0F);
  EXPECT_TRUE(
      ContainsVisibleEntity(reconnect_snapshot->visible_entities, kMonsterId));
  EXPECT_EQ(player->session(), &second_session);
  EXPECT_FALSE(player->operations_frozen());

  const Scene* reconnected_scene = scene_manager.Find(1);
  ASSERT_NE(reconnected_scene, nullptr);
  EXPECT_EQ(reconnected_scene->EntityCount(), 2u);
  const std::optional<shared::ScenePosition> self_position =
      reconnected_scene->GetPosition(reconnect_snapshot->controlled_entity_id);
  ASSERT_TRUE(self_position.has_value());
  EXPECT_FLOAT_EQ(self_position->x, 6.0F);
  EXPECT_FLOAT_EQ(self_position->y, 4.0F);
}

}  // namespace
}  // namespace server
