#include "server/gm/gm_command_handler.h"

#include <optional>
#include <sstream>

#include "server/ecs/components.h"
#include "server/entity/entity_factory.h"

namespace server {

bool GmCommandHandler::Teleport(Scene* scene, Player* player,
                                const shared::ScenePosition& position) const {
  if (scene == nullptr || player == nullptr ||
      !player->controlled_entity_id().has_value()) {
    return false;
  }

  const std::optional<entt::entity> entity =
      scene->Find(*player->controlled_entity_id());
  if (!entity.has_value()) {
    return false;
  }

  if (!scene->registry().all_of<ecs::PositionComponent>(*entity)) {
    return false;
  }

  scene->registry().get<ecs::PositionComponent>(*entity).position = position;
  return true;
}

bool GmCommandHandler::SpawnMonster(
    Scene* scene, std::uint32_t monster_template_id, shared::EntityId entity_id,
    const shared::ScenePosition& position) const {
  if (scene == nullptr || monster_template_id == 0 || entity_id.value == 0) {
    return false;
  }

  EntityFactory(scene).SpawnMonster(monster_template_id, entity_id, position);
  return true;
}

bool GmCommandHandler::KillMonster(
    Scene* scene, shared::EntityId monster_entity_id, DropSystem* drop_system,
    const std::vector<DropItemSpec>& drop_items) const {
  if (scene == nullptr || drop_system == nullptr) {
    return false;
  }

  const std::vector<shared::EntityId> drop_entity_ids =
      drop_system->SpawnDropsForDefeatedMonster(scene, monster_entity_id,
                                                drop_items);
  return !drop_entity_ids.empty();
}

bool GmCommandHandler::AddItem(Player* player, std::uint32_t item_template_id,
                               std::uint32_t item_count,
                               std::uint32_t max_stack_size) const {
  if (player == nullptr || item_template_id == 0 || item_count == 0) {
    return false;
  }

  return player->mutable_data().inventory.AddStackableItem(
      item_template_id, item_count, max_stack_size);
}

std::string GmCommandHandler::QueryPlayerState(const Player& player) const {
  std::ostringstream stream;
  stream << "player_id=" << player.data().identity.player_id.value
         << " scene_id=" << player.current_scene_id()
         << " dirty=" << player.dirty();
  return stream.str();
}

bool GmCommandHandler::ForceSave(Player* player, const Scene* scene,
                                 SaveService* save_service) const {
  if (player == nullptr || save_service == nullptr) {
    return false;
  }
  if (!player->CaptureSceneSnapshot(scene)) {
    return false;
  }

  save_service->QueueSnapshotFromMainThread(player->data());
  player->ClearDirty();
  return true;
}

}  // namespace server
