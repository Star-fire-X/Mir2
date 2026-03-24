#ifndef SERVER_BATTLE_BATTLE_SYSTEM_H_
#define SERVER_BATTLE_BATTLE_SYSTEM_H_

#include <cstdint>

#include "server/ecs/components.h"
#include "shared/protocol/combat_messages.h"

namespace server {

class BattleSystem {
 public:
  [[nodiscard]] shared::CastSkillResult ResolveSkillHit(
      const shared::CastSkillRequest& request, std::int32_t raw_damage,
      ecs::CombatStateComponent* target_combat_state) const;
};

}  // namespace server

#endif  // SERVER_BATTLE_BATTLE_SYSTEM_H_
