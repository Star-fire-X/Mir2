#include "server/aoi/aoi_system.h"

#include <cmath>
#include <unordered_set>

namespace server {

AoiSystem::AoiSystem(float visible_radius) : visible_radius_(visible_radius) {}

shared::EnterSceneSnapshot AoiSystem::BuildEnterSceneSnapshot(
    shared::PlayerId player_id, shared::EntityId self_entity_id,
    std::uint32_t scene_id, const Scene& scene) const {
  shared::EnterSceneSnapshot snapshot;
  snapshot.player_id = player_id;
  snapshot.controlled_entity_id = self_entity_id;
  snapshot.scene_id = scene_id;

  const std::optional<shared::ScenePosition> self_position =
      scene.GetPosition(self_entity_id);
  if (!self_position.has_value()) {
    return snapshot;
  }
  snapshot.self_position = *self_position;

  for (const shared::EntityId entity_id : scene.EntityIds()) {
    const std::optional<shared::VisibleEntitySnapshot> entity_snapshot =
        scene.BuildVisibleSnapshot(entity_id);
    if (!entity_snapshot.has_value()) {
      continue;
    }
    if (entity_id == self_entity_id ||
        IsVisible(*self_position, entity_snapshot->position)) {
      snapshot.visible_entities.push_back(*entity_snapshot);
    }
  }

  return snapshot;
}

AoiDiff AoiSystem::ComputeEnterLeave(shared::EntityId observer_entity_id,
                                     const shared::ScenePosition& old_position,
                                     const shared::ScenePosition& new_position,
                                     const Scene& scene) const {
  std::unordered_set<std::uint64_t> old_visible;
  std::unordered_set<std::uint64_t> new_visible;
  AoiDiff diff;

  for (const shared::EntityId entity_id : scene.EntityIds()) {
    if (entity_id == observer_entity_id) {
      continue;
    }

    const std::optional<shared::VisibleEntitySnapshot> entity_snapshot =
        scene.BuildVisibleSnapshot(entity_id);
    if (!entity_snapshot.has_value()) {
      continue;
    }

    if (IsVisible(old_position, entity_snapshot->position)) {
      old_visible.insert(entity_id.value);
    }
    if (IsVisible(new_position, entity_snapshot->position)) {
      new_visible.insert(entity_id.value);
    }
  }

  for (const shared::EntityId entity_id : scene.EntityIds()) {
    if (entity_id == observer_entity_id) {
      continue;
    }

    const bool in_old = old_visible.contains(entity_id.value);
    const bool in_new = new_visible.contains(entity_id.value);
    if (in_new && !in_old) {
      const std::optional<shared::VisibleEntitySnapshot> entity_snapshot =
          scene.BuildVisibleSnapshot(entity_id);
      if (entity_snapshot.has_value()) {
        diff.entered.push_back(*entity_snapshot);
      }
    } else if (in_old && !in_new) {
      diff.left.push_back(entity_id);
    }
  }

  return diff;
}

bool AoiSystem::IsVisible(const shared::ScenePosition& from,
                          const shared::ScenePosition& to) const {
  const float dx = from.x - to.x;
  const float dy = from.y - to.y;
  const float distance_squared = (dx * dx) + (dy * dy);
  const float radius_squared = visible_radius_ * visible_radius_;
  return distance_squared <= radius_squared;
}

}  // namespace server
