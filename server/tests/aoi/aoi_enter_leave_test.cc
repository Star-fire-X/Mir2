#include <algorithm>
#include <vector>

#include "gtest/gtest.h"
#include "server/aoi/aoi_system.h"
#include "server/data/character_data.h"
#include "server/entity/entity_factory.h"
#include "server/scene/scene.h"

namespace server {
namespace {

CharacterData MakeCharacter(shared::PlayerId player_id, float x, float y) {
  CharacterData character;
  character.identity.player_id = player_id;
  character.last_scene_snapshot.scene_id = 1;
  character.last_scene_snapshot.position = {x, y};
  return character;
}

bool ContainsEntityId(const std::vector<shared::EntityId>& entity_ids,
                      shared::EntityId expected) {
  return std::find(entity_ids.begin(), entity_ids.end(), expected) !=
         entity_ids.end();
}

bool ContainsSnapshotEntityId(
    const std::vector<shared::VisibleEntitySnapshot>& snapshots,
    shared::EntityId expected) {
  return std::find_if(
             snapshots.begin(), snapshots.end(),
             [expected](const shared::VisibleEntitySnapshot& snapshot) {
               return snapshot.entity_id == expected;
             }) != snapshots.end();
}

TEST(AoiEnterLeaveTest, MovingAcrossBoundaryProducesEnterAndLeaveDiff) {
  Scene scene;
  EntityFactory entity_factory(&scene);

  const shared::EntityId observer_entity_id{1001};
  entity_factory.SpawnPlayer(MakeCharacter(shared::PlayerId{1}, 0.0F, 0.0F),
                             observer_entity_id);

  const shared::EntityId near_monster_entity_id{2001};
  entity_factory.SpawnMonster(1, near_monster_entity_id,
                              shared::ScenePosition{2.0F, 0.0F});

  const shared::EntityId far_monster_entity_id{2002};
  entity_factory.SpawnMonster(2, far_monster_entity_id,
                              shared::ScenePosition{22.0F, 0.0F});

  AoiSystem aoi_system(10.0F);
  const AoiDiff diff = aoi_system.ComputeEnterLeave(
      observer_entity_id, shared::ScenePosition{0.0F, 0.0F},
      shared::ScenePosition{20.0F, 0.0F}, scene);

  EXPECT_TRUE(ContainsSnapshotEntityId(diff.entered, far_monster_entity_id));
  EXPECT_TRUE(ContainsEntityId(diff.left, near_monster_entity_id));
}

}  // namespace
}  // namespace server
