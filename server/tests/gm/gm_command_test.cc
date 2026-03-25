#include "gtest/gtest.h"
#include "server/data/character_data.h"
#include "server/db/save_service.h"
#include "server/ecs/components.h"
#include "server/entity/entity_factory.h"
#include "server/gm/gm_command_handler.h"
#include "server/player/player.h"
#include "server/scene/scene.h"

namespace server {
namespace {

CharacterData MakePlayerData(shared::PlayerId player_id) {
  CharacterData data;
  data.identity.player_id = player_id;
  data.identity.character_name = "gm_target";
  data.base_attributes.level = 10;
  data.base_attributes.vitality = 8;
  data.inventory = Inventory(4);
  data.last_scene_snapshot.scene_id = 1;
  data.last_scene_snapshot.position = {2.0F, 3.0F};
  return data;
}

TEST(GmCommandTest, SpawnMonsterCreatesMonsterEntityInScene) {
  Scene scene;
  GmCommandHandler handler;

  ASSERT_TRUE(handler.SpawnMonster(&scene, /*monster_template_id=*/2001,
                                   shared::EntityId{880001},
                                   shared::ScenePosition{7.0F, 4.0F}));

  ASSERT_EQ(scene.EntityCount(), 1U);
  const std::optional<shared::VisibleEntitySnapshot> snapshot =
      scene.BuildVisibleSnapshot(shared::EntityId{880001});
  ASSERT_TRUE(snapshot.has_value());
  EXPECT_EQ(snapshot->kind, shared::VisibleEntityKind::kMonster);
}

TEST(GmCommandTest, AddItemAndForceSaveQueueSnapshotFromLiveSceneState) {
  Scene scene;
  EntityFactory entity_factory(&scene);
  Player player(MakePlayerData(shared::PlayerId{77}));
  SaveService save_service;
  GmCommandHandler handler;

  const shared::EntityId controlled_entity_id{770001};
  entity_factory.SpawnPlayer(player.data(), controlled_entity_id);
  player.SetControlledEntity(controlled_entity_id, 1);

  const std::optional<entt::entity> entity = scene.Find(controlled_entity_id);
  ASSERT_TRUE(entity.has_value());
  scene.registry().get<ecs::PositionComponent>(*entity).position = {5.0F, 6.0F};

  ASSERT_TRUE(
      handler.AddItem(&player, /*item_template_id=*/5001, /*item_count=*/2, 20));
  ASSERT_TRUE(handler.ForceSave(&player, &scene, &save_service));

  ASSERT_TRUE(save_service.HasQueuedSnapshot());
  ASSERT_TRUE(player.data().inventory.slots()[0].has_value());
  EXPECT_EQ(player.data().inventory.slots()[0]->item_template_id, 5001U);
  EXPECT_EQ(player.data().inventory.slots()[0]->item_count, 2U);
  EXPECT_EQ(save_service.queued_snapshot().data.last_scene_snapshot.scene_id, 1U);
  EXPECT_FLOAT_EQ(
      save_service.queued_snapshot().data.last_scene_snapshot.position.x, 5.0F);
  EXPECT_FLOAT_EQ(
      save_service.queued_snapshot().data.last_scene_snapshot.position.y, 6.0F);
}

}  // namespace
}  // namespace server
