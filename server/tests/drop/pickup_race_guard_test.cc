#include <vector>

#include "gtest/gtest.h"
#include "server/data/character_data.h"
#include "server/player/player.h"
#include "server/scene/drop_system.h"
#include "server/scene/scene.h"
#include "shared/protocol/scene_messages.h"

namespace server {
namespace {

CharacterData MakePlayerData(shared::PlayerId player_id) {
  CharacterData data;
  data.identity.player_id = player_id;
  data.identity.character_name = "picker";
  data.base_attributes.level = 1;
  data.inventory = Inventory(4);
  data.last_scene_snapshot.scene_id = 1;
  return data;
}

TEST(PickupRaceGuardTest, FirstPickupWinsAndSecondPlayerSeesDropMissing) {
  Scene scene;
  DropSystem drop_system;
  Player first_player(MakePlayerData(shared::PlayerId{88}));
  Player second_player(MakePlayerData(shared::PlayerId{89}));

  const std::vector<shared::EntityId> drop_entity_ids = drop_system.SpawnDrops(
      &scene, shared::ScenePosition{1.0F, 2.0F}, {DropItemSpec{5001, 1}});
  ASSERT_EQ(drop_entity_ids.size(), 1U);

  const shared::PickupResult first_result =
      drop_system.HandlePickup(&scene, &first_player,
                               shared::PickupRequest{
                                   first_player.data().identity.player_id,
                                   drop_entity_ids[0],
                                   41,
                               },
                               20);
  const shared::PickupResult second_result =
      drop_system.HandlePickup(&scene, &second_player,
                               shared::PickupRequest{
                                   second_player.data().identity.player_id,
                                   drop_entity_ids[0],
                                   42,
                               },
                               20);

  EXPECT_EQ(first_result.error_code, shared::ErrorCode::kOk);
  EXPECT_EQ(second_result.error_code, shared::ErrorCode::kDropNotFound);
  ASSERT_TRUE(first_player.data().inventory.slots()[0].has_value());
  EXPECT_FALSE(second_player.data().inventory.slots()[0].has_value());
}

}  // namespace
}  // namespace server
