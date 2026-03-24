#include <algorithm>
#include <optional>

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

bool ContainsEntity(const shared::EnterSceneSnapshot& snapshot,
                    shared::EntityId entity_id) {
  return std::find_if(snapshot.visible_entities.begin(),
                      snapshot.visible_entities.end(),
                      [entity_id](const shared::VisibleEntitySnapshot& entity) {
                        return entity.entity_id == entity_id;
                      }) != snapshot.visible_entities.end();
}

TEST(EnterSceneSnapshotTest, SnapshotContainsSelfAndVisibleEntities) {
  Scene scene;
  EntityFactory entity_factory(&scene);
  AoiSystem aoi_system(10.0F);

  const shared::EntityId self_entity_id{3001};
  entity_factory.SpawnPlayer(MakeCharacter(shared::PlayerId{11}, 0.0F, 0.0F),
                             self_entity_id);

  const shared::EntityId near_monster_entity_id{3002};
  entity_factory.SpawnMonster(7, near_monster_entity_id,
                              shared::ScenePosition{3.0F, 4.0F});

  const shared::EntityId far_monster_entity_id{3003};
  entity_factory.SpawnMonster(9, far_monster_entity_id,
                              shared::ScenePosition{30.0F, 0.0F});

  const shared::EnterSceneSnapshot snapshot =
      aoi_system.BuildEnterSceneSnapshot(shared::PlayerId{11}, self_entity_id,
                                         1, scene);

  EXPECT_EQ(snapshot.player_id, shared::PlayerId{11});
  EXPECT_EQ(snapshot.controlled_entity_id, self_entity_id);
  EXPECT_EQ(snapshot.scene_id, 1U);
  EXPECT_EQ(snapshot.self_position.x, 0.0F);
  EXPECT_EQ(snapshot.self_position.y, 0.0F);
  EXPECT_TRUE(ContainsEntity(snapshot, self_entity_id));
  EXPECT_TRUE(ContainsEntity(snapshot, near_monster_entity_id));
  EXPECT_FALSE(ContainsEntity(snapshot, far_monster_entity_id));
}

}  // namespace
}  // namespace server
