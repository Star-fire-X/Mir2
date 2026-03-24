#include "server/scene/drop_system.h"

#include <optional>
#include <utility>
#include <vector>

#include "server/ecs/components.h"

namespace server {

DropSystem::DropSystem(std::uint64_t first_drop_entity_id)
    : next_drop_entity_id_(first_drop_entity_id) {}

std::vector<shared::EntityId> DropSystem::SpawnDrops(
    Scene* scene, const shared::ScenePosition& drop_position,
    const std::vector<DropItemSpec>& drop_items) {
  std::vector<shared::EntityId> spawned_drop_ids;
  if (scene == nullptr) {
    return spawned_drop_ids;
  }

  for (const DropItemSpec& drop_item : drop_items) {
    if (drop_item.item_template_id == 0 || drop_item.item_count == 0) {
      continue;
    }

    const shared::EntityId drop_entity_id{next_drop_entity_id_++};
    const entt::entity entity = scene->registry().create();
    scene->registry().emplace<ecs::IdentityComponent>(
        entity, ecs::IdentityComponent{drop_entity_id});
    scene->registry().emplace<ecs::PositionComponent>(
        entity, ecs::PositionComponent{drop_position});
    scene->registry().emplace<ecs::DropRefComponent>(
        entity, ecs::DropRefComponent{drop_item.item_template_id,
                                      drop_item.item_count, false});

    scene->RegisterEntity(drop_entity_id, entity);
    drops_[drop_entity_id] = DropEntry{
        drop_item.item_template_id,
        drop_item.item_count,
    };
    spawned_drop_ids.push_back(drop_entity_id);
  }

  return spawned_drop_ids;
}

std::vector<shared::EntityId> DropSystem::SpawnDropsForDefeatedMonster(
    Scene* scene, shared::EntityId defeated_monster_entity_id,
    const std::vector<DropItemSpec>& drop_items) {
  if (scene == nullptr) {
    return {};
  }

  const std::optional<shared::ScenePosition> monster_position =
      scene->GetPosition(defeated_monster_entity_id);
  if (!monster_position.has_value()) {
    return {};
  }

  scene->DestroyEntity(defeated_monster_entity_id);
  return SpawnDrops(scene, *monster_position, drop_items);
}

shared::PickupResult DropSystem::HandlePickup(
    Scene* scene, Player* player, const shared::PickupRequest& pickup_request,
    std::uint32_t max_stack_size) {
  shared::PickupResult result;
  result.player_id = pickup_request.player_id;
  result.drop_entity_id = pickup_request.drop_entity_id;
  result.client_seq = pickup_request.client_seq;
  result.error_code = shared::ErrorCode::kOk;

  if (scene == nullptr || player == nullptr ||
      player->data().identity.player_id != pickup_request.player_id) {
    result.error_code = shared::ErrorCode::kInvalidSkillTarget;
    return result;
  }

  const PickupCacheKey key{
      pickup_request.player_id.value,
      pickup_request.drop_entity_id.value,
      pickup_request.client_seq,
  };
  if (pickup_cache_.contains(key)) {
    result.error_code = shared::ErrorCode::kOk;
    return result;
  }

  const auto drop_it = drops_.find(pickup_request.drop_entity_id);
  if (drop_it == drops_.end()) {
    result.error_code = shared::ErrorCode::kDropNotFound;
    return result;
  }

  const DropEntry& drop = drop_it->second;
  const bool added = player->mutable_data().inventory.AddStackableItem(
      drop.item_template_id, drop.item_count, max_stack_size);
  if (!added) {
    result.error_code = shared::ErrorCode::kInventoryFull;
    return result;
  }

  pickup_cache_.insert(key);
  drops_.erase(drop_it);
  scene->DestroyEntity(pickup_request.drop_entity_id);
  return result;
}

}  // namespace server
