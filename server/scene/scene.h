#ifndef SERVER_SCENE_SCENE_H_
#define SERVER_SCENE_SCENE_H_

#include <cstddef>
#include <cstdint>
#include <optional>
#include <queue>
#include <unordered_map>
#include <vector>

#include "entt/entity/fwd.hpp"
#include "entt/entity/registry.hpp"
#include "shared/protocol/scene_messages.h"
#include "shared/types/entity_id.h"

namespace server {

class MovementSystem;

enum class SceneCommandType : std::uint8_t {
  kUnknown = 0,
  kMove = 1,
};

struct SceneCommand {
  SceneCommandType type = SceneCommandType::kUnknown;
  shared::MoveRequest move_request{};
};

class Scene {
 public:
  Scene();

  void Enqueue(SceneCommand command);
  bool HasPendingCommands() const;
  std::optional<SceneCommand> Dequeue();
  void Tick(MovementSystem* movement_system, float delta_seconds);

  bool RegisterEntity(shared::EntityId entity_id, entt::entity entity);
  bool RemoveEntity(shared::EntityId entity_id);
  std::optional<entt::entity> Find(shared::EntityId entity_id) const;
  std::size_t EntityCount() const;
  std::vector<shared::EntityId> EntityIds() const;
  std::optional<shared::ScenePosition> GetPosition(
      shared::EntityId entity_id) const;
  std::optional<shared::VisibleEntitySnapshot> BuildVisibleSnapshot(
      shared::EntityId entity_id) const;

  const std::vector<shared::MoveCorrection>& recent_move_corrections() const;
  void ClearRecentMoveCorrections();

  entt::registry& registry();
  const entt::registry& registry() const;

 private:
  struct EntityIdHash {
    std::size_t operator()(const shared::EntityId& entity_id) const noexcept;
  };

  entt::registry registry_;
  std::queue<SceneCommand> command_queue_;
  std::unordered_map<shared::EntityId, entt::entity, EntityIdHash>
      entity_index_;
  std::vector<shared::MoveCorrection> recent_move_corrections_;
};

}  // namespace server

#endif  // SERVER_SCENE_SCENE_H_
