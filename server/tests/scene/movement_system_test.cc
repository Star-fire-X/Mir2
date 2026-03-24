#include "server/scene/movement_system.h"

#include <optional>

#include "gtest/gtest.h"
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

TEST(MovementSystemTest, LegalMoveAdvancesPosition) {
  Scene scene;
  EntityFactory entity_factory(&scene);
  const shared::EntityId entity_id{4001};
  entity_factory.SpawnPlayer(MakeCharacter(shared::PlayerId{21}, 0.0F, 0.0F),
                             entity_id);

  MovementSystem movement_system(6.0F, 0.1F);
  std::optional<shared::MoveCorrection> correction;

  const bool accepted = movement_system.ApplyMove(
      scene,
      shared::MoveRequest{entity_id, shared::ScenePosition{3.0F, 0.0F}, 1, 1},
      1.0F, &correction);

  ASSERT_TRUE(accepted);
  EXPECT_FALSE(correction.has_value());
  const std::optional<shared::ScenePosition> position =
      scene.GetPosition(entity_id);
  ASSERT_TRUE(position.has_value());
  EXPECT_EQ(position->x, 3.0F);
  EXPECT_EQ(position->y, 0.0F);
}

TEST(MovementSystemTest, OverspeedMoveIsRejectedWithCorrection) {
  Scene scene;
  EntityFactory entity_factory(&scene);
  const shared::EntityId entity_id{4002};
  entity_factory.SpawnPlayer(MakeCharacter(shared::PlayerId{22}, 0.0F, 0.0F),
                             entity_id);

  MovementSystem movement_system(6.0F, 0.1F);
  std::optional<shared::MoveCorrection> correction;

  const bool accepted = movement_system.ApplyMove(
      scene,
      shared::MoveRequest{entity_id, shared::ScenePosition{20.0F, 0.0F}, 2, 2},
      1.0F, &correction);

  EXPECT_FALSE(accepted);
  ASSERT_TRUE(correction.has_value());
  EXPECT_EQ(correction->entity_id, entity_id);
  EXPECT_EQ(correction->authoritative_position.x, 0.0F);
  EXPECT_EQ(correction->authoritative_position.y, 0.0F);
}

TEST(MovementSystemTest, BlockedTileMoveIsRejectedWithCorrection) {
  Scene scene;
  EntityFactory entity_factory(&scene);
  const shared::EntityId entity_id{4003};
  entity_factory.SpawnPlayer(MakeCharacter(shared::PlayerId{23}, 0.0F, 0.0F),
                             entity_id);

  MovementSystem movement_system(6.0F, 0.1F);
  movement_system.AddBlockedRect(
      MovementSystem::BlockedRect{3.5F, -1.0F, 4.5F, 1.0F});
  std::optional<shared::MoveCorrection> correction;

  const bool accepted = movement_system.ApplyMove(
      scene,
      shared::MoveRequest{entity_id, shared::ScenePosition{4.0F, 0.0F}, 3, 3},
      1.0F, &correction);

  EXPECT_FALSE(accepted);
  ASSERT_TRUE(correction.has_value());
  EXPECT_EQ(correction->authoritative_position.x, 0.0F);
  EXPECT_EQ(correction->authoritative_position.y, 0.0F);
}

}  // namespace
}  // namespace server
