#include "server/scene/movement_system.h"

#include <algorithm>
#include <cmath>
#include <optional>

#include "server/ecs/components.h"

namespace server {

MovementSystem::MovementSystem(float max_speed_units_per_second,
                               float correction_threshold) {
  max_speed_units_per_second_ = max_speed_units_per_second;
  correction_threshold_ = correction_threshold;
}

void MovementSystem::AddBlockedRect(BlockedRect blocked_rect) {
  blocked_rects_.push_back(blocked_rect);
}

float MovementSystem::max_speed_units_per_second() const {
  return max_speed_units_per_second_;
}

float MovementSystem::correction_threshold() const {
  return correction_threshold_;
}

bool MovementSystem::ApplyMove(
    Scene* scene, const shared::MoveRequest& move_request, float delta_seconds,
    std::optional<shared::MoveCorrection>* correction) const {
  if (correction != nullptr) {
    correction->reset();
  }
  if (scene == nullptr) {
    return false;
  }

  const std::optional<entt::entity> entity =
      scene->Find(move_request.entity_id);
  if (!entity.has_value()) {
    return false;
  }

  auto& registry = scene->registry();
  if (!registry.all_of<ecs::PositionComponent>(*entity)) {
    return false;
  }

  ecs::PositionComponent& position =
      registry.get<ecs::PositionComponent>(*entity);
  const float dx = move_request.target_position.x - position.position.x;
  const float dy = move_request.target_position.y - position.position.y;
  const float distance = std::sqrt((dx * dx) + (dy * dy));
  const float max_distance = max_speed_units_per_second_ * delta_seconds;

  const bool speed_ok = distance <= max_distance + correction_threshold_;
  const bool blocked = IsBlocked(move_request.target_position);
  if (!speed_ok || blocked) {
    if (correction != nullptr) {
      *correction = shared::MoveCorrection{
          move_request.entity_id,
          position.position,
          move_request.client_seq,
      };
    }
    return false;
  }

  position.position = move_request.target_position;
  if (registry.all_of<ecs::MoveStateComponent>(*entity)) {
    ecs::MoveStateComponent& move_state =
        registry.get<ecs::MoveStateComponent>(*entity);
    move_state.target_position = move_request.target_position;
    move_state.moving = false;
  }

  return true;
}

bool MovementSystem::IsBlocked(const shared::ScenePosition& position) const {
  return std::any_of(blocked_rects_.begin(), blocked_rects_.end(),
                     [&position](const BlockedRect& blocked_rect) {
                       return position.x >= blocked_rect.min_x &&
                              position.x <= blocked_rect.max_x &&
                              position.y >= blocked_rect.min_y &&
                              position.y <= blocked_rect.max_y;
                     });
}

}  // namespace server
