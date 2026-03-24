#include "server/battle/battle_system.h"

#include <algorithm>
#include <cstdint>
#include <limits>

namespace server {

shared::CastSkillResult BattleSystem::ResolveSkillHit(
    const shared::CastSkillRequest& request, std::int32_t raw_damage,
    ecs::CombatStateComponent* target_combat_state) const {
  shared::CastSkillResult result;
  result.caster_entity_id = request.caster_entity_id;
  result.target_entity_id = request.target_entity_id;
  result.skill_id = request.skill_id;
  result.client_seq = request.client_seq;
  result.error_code = shared::ErrorCode::kOk;
  result.hp_delta = 0;

  if (target_combat_state == nullptr || target_combat_state->is_dead) {
    result.error_code = shared::ErrorCode::kInvalidSkillTarget;
    return result;
  }

  const std::uint32_t damage =
      static_cast<std::uint32_t>(std::max(raw_damage, 0));
  const std::uint32_t next_hp = target_combat_state->current_hp > damage
                                    ? target_combat_state->current_hp - damage
                                    : 0U;
  target_combat_state->current_hp = next_hp;
  target_combat_state->is_dead = next_hp == 0;

  constexpr std::int32_t kMaxPositiveDamage =
      std::numeric_limits<std::int32_t>::max();
  const std::int32_t bounded_damage =
      damage > static_cast<std::uint32_t>(kMaxPositiveDamage)
          ? kMaxPositiveDamage
          : static_cast<std::int32_t>(damage);
  result.hp_delta = -bounded_damage;
  return result;
}

}  // namespace server
