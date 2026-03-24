#include "server/entity/entity_factory.h"

#include <stdexcept>

#include "server/ecs/components.h"
#include "server/scene/scene.h"

namespace server {

EntityFactory::EntityFactory(Scene* scene) : scene_(scene) {
  if (scene_ == nullptr) {
    throw std::invalid_argument("scene pointer must not be null");
  }
}

entt::entity EntityFactory::SpawnPlayer(const CharacterData& character_data,
                                        shared::EntityId entity_id) {
  const shared::ScenePosition spawn_position{
      character_data.last_scene_snapshot.position.x,
      character_data.last_scene_snapshot.position.y};
  const entt::entity entity = SpawnBaseEntity(entity_id, spawn_position);
  auto& registry = scene_->registry();

  const auto& attributes = character_data.base_attributes;
  registry.emplace<ecs::AttributeComponent>(
      entity, ecs::AttributeComponent{attributes.level, attributes.strength,
                                      attributes.agility, attributes.vitality,
                                      attributes.intelligence});

  const std::uint32_t max_hp = 100U + (attributes.vitality * 10U);
  registry.emplace<ecs::CombatStateComponent>(
      entity, ecs::CombatStateComponent{max_hp, max_hp, false});
  registry.emplace<ecs::PlayerRefComponent>(
      entity, ecs::PlayerRefComponent{character_data.identity.player_id});
  return entity;
}

entt::entity EntityFactory::SpawnMonster(
    std::uint32_t monster_template_id, shared::EntityId entity_id,
    const shared::ScenePosition& spawn_position) {
  const entt::entity entity = SpawnBaseEntity(entity_id, spawn_position);
  auto& registry = scene_->registry();

  registry.emplace<ecs::AttributeComponent>(
      entity, ecs::AttributeComponent{1, 8, 6, 5, 2});
  registry.emplace<ecs::CombatStateComponent>(
      entity, ecs::CombatStateComponent{80, 80, false});
  registry.emplace<ecs::MonsterRefComponent>(
      entity, ecs::MonsterRefComponent{monster_template_id});
  registry.emplace<ecs::AiStateComponent>(entity, ecs::AiStateComponent{true});
  registry.emplace<ecs::HateListComponent>(entity, ecs::HateListComponent{});
  return entity;
}

entt::entity EntityFactory::SpawnBaseEntity(
    shared::EntityId entity_id, const shared::ScenePosition& spawn_position) {
  auto& registry = scene_->registry();
  const entt::entity entity = registry.create();
  registry.emplace<ecs::IdentityComponent>(entity,
                                           ecs::IdentityComponent{entity_id});
  registry.emplace<ecs::PositionComponent>(
      entity, ecs::PositionComponent{spawn_position});
  registry.emplace<ecs::MoveStateComponent>(
      entity, ecs::MoveStateComponent{spawn_position, false});
  registry.emplace<ecs::SkillRuntimeComponent>(entity,
                                               ecs::SkillRuntimeComponent{});
  registry.emplace<ecs::BuffContainerComponent>(entity,
                                                ecs::BuffContainerComponent{});
  registry.emplace<ecs::AoiStateComponent>(entity, ecs::AoiStateComponent{});

  scene_->RegisterEntity(entity_id, entity);
  return entity;
}

}  // namespace server
