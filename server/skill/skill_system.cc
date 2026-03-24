#include "server/skill/skill_system.h"

#include <optional>

namespace server {

void SkillSystem::RegisterSkill(SkillDefinition definition) {
  skill_definitions_[definition.skill_id] = definition;
}

shared::ErrorCode SkillSystem::ValidateAndConsume(
    const shared::CastSkillRequest& request, float target_distance,
    float now_seconds, SkillRuntimeState* runtime_state) const {
  if (runtime_state == nullptr) {
    return shared::ErrorCode::kInvalidSkillTarget;
  }

  const std::optional<SkillDefinition> skill = FindSkill(request.skill_id);
  if (!skill.has_value()) {
    return shared::ErrorCode::kInvalidSkillTarget;
  }
  if (request.target_entity_id.value == 0) {
    return shared::ErrorCode::kInvalidSkillTarget;
  }

  const auto cooldown_it =
      runtime_state->cooldown_until_by_skill_id.find(request.skill_id);
  if (cooldown_it != runtime_state->cooldown_until_by_skill_id.end() &&
      now_seconds < cooldown_it->second) {
    return shared::ErrorCode::kSkillOnCooldown;
  }

  if (target_distance > skill->max_cast_range) {
    return shared::ErrorCode::kOutOfRange;
  }
  if (runtime_state->current_mp < skill->mana_cost) {
    return shared::ErrorCode::kInvalidSkillTarget;
  }

  runtime_state->current_mp -= skill->mana_cost;
  runtime_state->cooldown_until_by_skill_id[request.skill_id] =
      now_seconds + skill->cooldown_seconds;
  return shared::ErrorCode::kOk;
}

std::optional<SkillDefinition> SkillSystem::FindSkill(
    std::uint32_t skill_id) const {
  const auto it = skill_definitions_.find(skill_id);
  if (it == skill_definitions_.end()) {
    return std::nullopt;
  }
  return it->second;
}

}  // namespace server
