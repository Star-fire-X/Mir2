#include "server/scene/scene.h"

#include <cstdint>
#include <functional>
#include <utility>

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

entt::registry& Scene::registry() { return registry_; }

const entt::registry& Scene::registry() const { return registry_; }

std::size_t Scene::EntityIdHash::operator()(
    const shared::EntityId& entity_id) const noexcept {
  return std::hash<std::uint64_t>{}(entity_id.value);
}

}  // namespace server
