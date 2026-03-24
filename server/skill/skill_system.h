#ifndef SERVER_SKILL_SKILL_SYSTEM_H_
#define SERVER_SKILL_SKILL_SYSTEM_H_

#include <cstdint>
#include <optional>
#include <unordered_map>

#include "shared/protocol/combat_messages.h"
#include "shared/types/error_codes.h"

namespace server {

struct SkillDefinition {
  std::uint32_t skill_id = 0;
  float max_cast_range = 0.0F;
  std::uint32_t mana_cost = 0;
  float cooldown_seconds = 0.0F;
  std::int32_t base_damage = 0;
};

struct SkillRuntimeState {
  std::uint32_t current_mp = 0;
  std::unordered_map<std::uint32_t, float> cooldown_until_by_skill_id;
};

class SkillSystem {
 public:
  void RegisterSkill(SkillDefinition definition);

  [[nodiscard]] shared::ErrorCode ValidateAndConsume(
      const shared::CastSkillRequest& request, float target_distance,
      float now_seconds, SkillRuntimeState* runtime_state) const;

  [[nodiscard]] std::optional<SkillDefinition> FindSkill(
      std::uint32_t skill_id) const;

 private:
  std::unordered_map<std::uint32_t, SkillDefinition> skill_definitions_;
};

}  // namespace server

#endif  // SERVER_SKILL_SKILL_SYSTEM_H_
