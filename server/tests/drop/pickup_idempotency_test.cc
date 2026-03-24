#include <vector>

#include "gtest/gtest.h"
#include "server/data/character_data.h"
#include "server/entity/entity_factory.h"
#include "server/player/player.h"
#include "server/scene/drop_system.h"
#include "server/scene/scene.h"

namespace server {
namespace {

CharacterData MakePlayerData(shared::PlayerId player_id) {
  CharacterData data;
  data.identity.player_id = player_id;
  data.identity.character_name = "hero";
  data.base_attributes.level = 1;
  data.inventory = Inventory(4);
  data.last_scene_snapshot.scene_id = 1;
  return data;
}

TEST(PickupIdempotencyTest, DuplicatePickupRequestDoesNotDuplicateItems) {
  Scene scene;
  DropSystem drop_system;
  Player player(MakePlayerData(shared::PlayerId{77}));
  EntityFactory entity_factory(&scene);

  const shared::EntityId monster_entity_id{9001};
  entity_factory.SpawnMonster(2001, monster_entity_id,
                              shared::ScenePosition{1.0F, 2.0F});

  const std::vector<DropItemSpec> drops = {
      DropItemSpec{5001, 1},
  };
  const std::vector<shared::EntityId> drop_entity_ids =
      drop_system.SpawnDropsForDefeatedMonster(&scene, monster_entity_id,
                                               drops);
  ASSERT_EQ(drop_entity_ids.size(), 1U);

  const shared::PickupRequest pickup_request{
      player.data().identity.player_id,
      drop_entity_ids[0],
      42,
  };
  const shared::PickupResult first =
      drop_system.HandlePickup(&scene, &player, pickup_request, 20);
  const shared::PickupResult duplicate =
      drop_system.HandlePickup(&scene, &player, pickup_request, 20);

  EXPECT_EQ(first.error_code, shared::ErrorCode::kOk);
  EXPECT_EQ(duplicate.error_code, shared::ErrorCode::kOk);
  ASSERT_TRUE(player.data().inventory.slots()[0].has_value());
  EXPECT_EQ(player.data().inventory.slots()[0]->item_template_id, 5001U);
  EXPECT_EQ(player.data().inventory.slots()[0]->item_count, 1U);
}

TEST(PickupIdempotencyTest, PickupRejectsDropMissingFromProvidedScene) {
  Scene origin_scene;
  Scene unrelated_scene;
  DropSystem drop_system;
  Player player(MakePlayerData(shared::PlayerId{78}));

  const std::vector<DropItemSpec> drops = {
      DropItemSpec{5001, 1},
  };
  const std::vector<shared::EntityId> drop_entity_ids = drop_system.SpawnDrops(
      &origin_scene, shared::ScenePosition{1.0F, 2.0F}, drops);
  ASSERT_EQ(drop_entity_ids.size(), 1U);

  const shared::PickupRequest pickup_request{
      player.data().identity.player_id,
      drop_entity_ids[0],
      43,
  };
  const shared::PickupResult wrong_scene_result =
      drop_system.HandlePickup(&unrelated_scene, &player, pickup_request, 20);
  EXPECT_EQ(wrong_scene_result.error_code, shared::ErrorCode::kDropNotFound);
  EXPECT_FALSE(player.data().inventory.slots()[0].has_value());

  const shared::PickupResult correct_scene_result =
      drop_system.HandlePickup(&origin_scene, &player, pickup_request, 20);
  EXPECT_EQ(correct_scene_result.error_code, shared::ErrorCode::kOk);
  ASSERT_TRUE(player.data().inventory.slots()[0].has_value());
  EXPECT_EQ(player.data().inventory.slots()[0]->item_template_id, 5001U);
  EXPECT_EQ(origin_scene.EntityCount(), 0U);
}

}  // namespace
}  // namespace server
