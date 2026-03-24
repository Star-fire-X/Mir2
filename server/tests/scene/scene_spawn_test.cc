#include "gtest/gtest.h"
#include "server/data/character_data.h"
#include "server/ecs/components.h"
#include "server/entity/entity_factory.h"
#include "server/scene/scene.h"

namespace server {
namespace {

TEST(SceneSpawnTest, PlayerSpawnCreatesRequiredComponents) {
  Scene scene;
  EntityFactory factory(&scene);

  CharacterData character;
  character.identity.player_id = shared::PlayerId{1001};
  character.base_attributes.level = 7;
  character.base_attributes.strength = 10;
  character.base_attributes.agility = 8;
  character.base_attributes.vitality = 9;
  character.base_attributes.intelligence = 6;
  character.last_scene_snapshot.scene_id = 1;
  character.last_scene_snapshot.position = {10.0F, 20.0F};

  const shared::EntityId entity_id{9001};
  const auto entity = factory.SpawnPlayer(character, entity_id);
  auto& registry = scene.registry();

  EXPECT_TRUE(registry.all_of<ecs::IdentityComponent>(entity));
  EXPECT_TRUE(registry.all_of<ecs::PositionComponent>(entity));
  EXPECT_TRUE(registry.all_of<ecs::MoveStateComponent>(entity));
  EXPECT_TRUE(registry.all_of<ecs::AttributeComponent>(entity));
  EXPECT_TRUE(registry.all_of<ecs::CombatStateComponent>(entity));
  EXPECT_TRUE(registry.all_of<ecs::SkillRuntimeComponent>(entity));
  EXPECT_TRUE(registry.all_of<ecs::BuffContainerComponent>(entity));
  EXPECT_TRUE(registry.all_of<ecs::AoiStateComponent>(entity));
  EXPECT_TRUE(registry.all_of<ecs::PlayerRefComponent>(entity));
}

TEST(SceneSpawnTest, MonsterSpawnCreatesAiAndHateComponents) {
  Scene scene;
  EntityFactory factory(&scene);

  const shared::EntityId entity_id{9002};
  const auto entity = factory.SpawnMonster(
      /*monster_template_id=*/2001, entity_id,
      shared::ScenePosition{3.0F, 4.0F});
  auto& registry = scene.registry();

  EXPECT_TRUE(registry.all_of<ecs::IdentityComponent>(entity));
  EXPECT_TRUE(registry.all_of<ecs::PositionComponent>(entity));
  EXPECT_TRUE(registry.all_of<ecs::MonsterRefComponent>(entity));
  EXPECT_TRUE(registry.all_of<ecs::AiStateComponent>(entity));
  EXPECT_TRUE(registry.all_of<ecs::HateListComponent>(entity));
}

TEST(SceneSpawnTest, SceneStoresEntityIdMapping) {
  Scene scene;
  EntityFactory factory(&scene);

  CharacterData character;
  character.identity.player_id = shared::PlayerId{1002};

  const shared::EntityId entity_id{9003};
  const auto entity = factory.SpawnPlayer(character, entity_id);

  const auto found = scene.Find(entity_id);
  ASSERT_TRUE(found.has_value());
  EXPECT_EQ(found.value(), entity);
  EXPECT_FALSE(scene.Find(shared::EntityId{4040}).has_value());
}

}  // namespace
}  // namespace server
