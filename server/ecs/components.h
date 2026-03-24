#ifndef SERVER_ECS_COMPONENTS_H_
#define SERVER_ECS_COMPONENTS_H_

#include <cstdint>
#include <vector>

#include "shared/protocol/scene_messages.h"
#include "shared/types/entity_id.h"
#include "shared/types/player_id.h"

namespace server::ecs {

struct IdentityComponent {
  shared::EntityId entity_id{};
};

struct PositionComponent {
  shared::ScenePosition position{};
};

struct MoveStateComponent {
  shared::ScenePosition target_position{};
  bool moving = false;
};

struct AttributeComponent {
  std::uint32_t level = 1;
  std::uint32_t strength = 0;
  std::uint32_t agility = 0;
  std::uint32_t vitality = 0;
  std::uint32_t intelligence = 0;
};

struct CombatStateComponent {
  std::uint32_t current_hp = 0;
  std::uint32_t max_hp = 0;
  bool is_dead = false;
};

struct SkillRuntimeComponent {
  std::vector<std::uint32_t> cooldown_skill_ids;
};

struct BuffContainerComponent {
  std::vector<std::uint32_t> buff_ids;
};

struct AoiStateComponent {
  std::vector<shared::EntityId> visible_entity_ids;
};

struct PlayerRefComponent {
  shared::PlayerId player_id{};
};

struct MonsterRefComponent {
  std::uint32_t monster_template_id = 0;
};

struct AiStateComponent {
  bool active = true;
};

struct HateListComponent {
  std::vector<shared::EntityId> targets;
};

}  // namespace server::ecs

#endif  // SERVER_ECS_COMPONENTS_H_
