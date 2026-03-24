#include "gtest/gtest.h"
#include "server/data/character_data.h"
#include "server/entity/entity_factory.h"
#include "server/scene/movement_system.h"
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

TEST(MoveCorrectionTest, TickQueuesAuthoritativeCorrectionForRejectedMove) {
  Scene scene;
  EntityFactory entity_factory(&scene);
  const shared::EntityId entity_id{5001};
  entity_factory.SpawnPlayer(MakeCharacter(shared::PlayerId{31}, 0.0F, 0.0F),
                             entity_id);

  scene.Enqueue(SceneCommand{
      SceneCommandType::kMove,
      shared::MoveRequest{
          entity_id,
          shared::ScenePosition{25.0F, 0.0F},
          17,
          12345,
      },
  });

  MovementSystem movement_system(6.0F, 0.1F);
  scene.Tick(&movement_system, 1.0F);

  ASSERT_EQ(scene.recent_move_corrections().size(), 1U);
  const shared::MoveCorrection& correction = scene.recent_move_corrections()[0];
  EXPECT_EQ(correction.entity_id, entity_id);
  EXPECT_EQ(correction.client_seq, 17U);
  EXPECT_EQ(correction.authoritative_position.x, 0.0F);
  EXPECT_EQ(correction.authoritative_position.y, 0.0F);
}

TEST(MoveCorrectionTest, TickSharesSingleMovementBudgetAcrossQueuedMoves) {
  Scene scene;
  EntityFactory entity_factory(&scene);
  const shared::EntityId entity_id{5002};
  entity_factory.SpawnPlayer(MakeCharacter(shared::PlayerId{32}, 0.0F, 0.0F),
                             entity_id);

  for (int i = 0; i < 10; ++i) {
    scene.Enqueue(SceneCommand{
        SceneCommandType::kMove,
        shared::MoveRequest{
            entity_id,
            shared::ScenePosition{0.3F * static_cast<float>(i + 1), 0.0F},
            static_cast<std::uint32_t>(i + 1),
            static_cast<std::uint64_t>(i + 1),
        },
    });
  }

  MovementSystem movement_system(6.0F, 0.1F);
  scene.Tick(&movement_system, 0.05F);

  const std::optional<shared::ScenePosition> position =
      scene.GetPosition(entity_id);
  ASSERT_TRUE(position.has_value());
  EXPECT_FLOAT_EQ(position->x, 0.3F);
  EXPECT_FLOAT_EQ(position->y, 0.0F);
  EXPECT_EQ(scene.recent_move_corrections().size(), 9U);
}

}  // namespace
}  // namespace server
