#include "gtest/gtest.h"
#include "server/battle/battle_system.h"
#include "server/ecs/components.h"

namespace server {
namespace {

TEST(DamageResolutionTest, SkillHitReducesTargetHp) {
  BattleSystem battle_system;
  ecs::CombatStateComponent target;
  target.current_hp = 100;
  target.max_hp = 100;

  const shared::CastSkillRequest request{
      shared::EntityId{100},
      shared::EntityId{200},
      9001,
      1,
  };
  const shared::CastSkillResult result =
      battle_system.ResolveSkillHit(request, 35, &target);

  EXPECT_EQ(result.error_code, shared::ErrorCode::kOk);
  EXPECT_EQ(result.hp_delta, -35);
  EXPECT_EQ(target.current_hp, 65U);
  EXPECT_FALSE(target.is_dead);
}

}  // namespace
}  // namespace server
