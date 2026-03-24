#ifndef SERVER_ECS_COMPONENTS_H_
#define SERVER_ECS_COMPONENTS_H_

#include <cstdint>
#include <unordered_map>
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
  std::unordered_map<std::uint32_t, float> cooldown_until_by_skill_id;
  std::uint32_t current_mp = 0;
};

struct BuffContainerComponent {
  std::vector<std::uint32_t> buff_ids;
  std::unordered_map<std::uint32_t, float> remaining_seconds_by_buff_id;
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
  enum class State : std::uint8_t {
    kIdle = 0,
    kDetect = 1,
    kChase = 2,
    kAttack = 3,
    kDisengage = 4,
    kReturn = 5,
  };

  State state = State::kIdle;
  bool active = true;
};

struct HateListComponent {
  std::vector<shared::EntityId> targets;
};

struct DropRefComponent {
  std::uint32_t item_template_id = 0;
  std::uint32_t item_count = 0;
  bool picked = false;
};

}  // namespace server::ecs

#endif  // SERVER_ECS_COMPONENTS_H_
