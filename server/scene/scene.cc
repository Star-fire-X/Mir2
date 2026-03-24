#include "server/scene/scene.h"

#include <cstdint>
#include <functional>
#include <iterator>
#include <utility>
#include <vector>

#include "server/ecs/components.h"
#include "server/scene/movement_system.h"

namespace server {

Scene::Scene() = default;

void Scene::Enqueue(SceneCommand command) {
  command_queue_.push(std::move(command));
}

bool Scene::HasPendingCommands() const { return !command_queue_.empty(); }

std::optional<SceneCommand> Scene::Dequeue() {
  if (command_queue_.empty()) {
    return std::nullopt;
  }

  SceneCommand command = command_queue_.front();
  command_queue_.pop();
  return command;
}

void Scene::Tick(MovementSystem* movement_system, float delta_seconds) {
  recent_move_corrections_.clear();
  if (movement_system == nullptr) {
    return;
  }

  while (HasPendingCommands()) {
    const std::optional<SceneCommand> command = Dequeue();
    if (!command.has_value()) {
      break;
    }

    if (command->type != SceneCommandType::kMove) {
      continue;
    }

    std::optional<shared::MoveCorrection> correction;
    const bool accepted = movement_system->ApplyMove(
        *this, command->move_request, delta_seconds, &correction);
    if (!accepted && correction.has_value()) {
      recent_move_corrections_.push_back(*correction);
    }
  }
}

bool Scene::RegisterEntity(shared::EntityId entity_id, entt::entity entity) {
  const auto [_, inserted] = entity_index_.emplace(entity_id, entity);
  return inserted;
}

bool Scene::RemoveEntity(shared::EntityId entity_id) {
  return entity_index_.erase(entity_id) > 0;
}

std::optional<entt::entity> Scene::Find(shared::EntityId entity_id) const {
  const auto it = entity_index_.find(entity_id);
  if (it == entity_index_.end()) {
    return std::nullopt;
  }
  return it->second;
}

std::size_t Scene::EntityCount() const { return entity_index_.size(); }

std::vector<shared::EntityId> Scene::EntityIds() const {
  std::vector<shared::EntityId> entity_ids;
  entity_ids.reserve(entity_index_.size());
  for (const auto& [entity_id, _] : entity_index_) {
    entity_ids.push_back(entity_id);
  }
  return entity_ids;
}

std::optional<shared::ScenePosition> Scene::GetPosition(
    shared::EntityId entity_id) const {
  const std::optional<entt::entity> entity = Find(entity_id);
  if (!entity.has_value()) {
    return std::nullopt;
  }

  if (!registry_.all_of<ecs::PositionComponent>(*entity)) {
    return std::nullopt;
  }

  return registry_.get<ecs::PositionComponent>(*entity).position;
}

std::optional<shared::VisibleEntitySnapshot> Scene::BuildVisibleSnapshot(
    shared::EntityId entity_id) const {
  const std::optional<entt::entity> entity = Find(entity_id);
  if (!entity.has_value()) {
    return std::nullopt;
  }

  if (!registry_.all_of<ecs::PositionComponent>(*entity)) {
    return std::nullopt;
  }

  shared::VisibleEntityKind kind = shared::VisibleEntityKind::kUnknown;
  if (registry_.all_of<ecs::PlayerRefComponent>(*entity)) {
    kind = shared::VisibleEntityKind::kPlayer;
  } else if (registry_.all_of<ecs::MonsterRefComponent>(*entity)) {
    kind = shared::VisibleEntityKind::kMonster;
  }

  return shared::VisibleEntitySnapshot{
      entity_id,
      kind,
      registry_.get<ecs::PositionComponent>(*entity).position,
  };
}

const std::vector<shared::MoveCorrection>& Scene::recent_move_corrections()
    const {
  return recent_move_corrections_;
}

void Scene::ClearRecentMoveCorrections() { recent_move_corrections_.clear(); }

entt::registry& Scene::registry() { return registry_; }

const entt::registry& Scene::registry() const { return registry_; }

std::size_t Scene::EntityIdHash::operator()(
    const shared::EntityId& entity_id) const noexcept {
  return std::hash<std::uint64_t>{}(entity_id.value);
}

}  // namespace server
