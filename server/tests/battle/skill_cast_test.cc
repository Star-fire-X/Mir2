#include "gtest/gtest.h"
#include "server/skill/skill_system.h"

namespace server {
namespace {

TEST(SkillCastTest, CastFailsWhenSkillIsOnCooldown) {
  SkillSystem skill_system;
  SkillRuntimeState runtime;
  runtime.current_mp = 100;

  skill_system.RegisterSkill(SkillDefinition{1001, 5.0F, 10U, 3.0F, 20});
  const shared::CastSkillRequest request{
      shared::EntityId{1},
      shared::EntityId{2},
      1001,
      1,
  };

  EXPECT_EQ(skill_system.ValidateAndConsume(request, 2.0F, 0.0F, &runtime),
            shared::ErrorCode::kOk);
  EXPECT_EQ(skill_system.ValidateAndConsume(request, 2.0F, 1.0F, &runtime),
            shared::ErrorCode::kSkillOnCooldown);
}

TEST(SkillCastTest, CastFailsWhenTargetIsOutOfRange) {
  SkillSystem skill_system;
  SkillRuntimeState runtime;
  runtime.current_mp = 100;

  skill_system.RegisterSkill(SkillDefinition{1002, 4.0F, 5U, 1.0F, 10});
  const shared::CastSkillRequest request{
      shared::EntityId{1},
      shared::EntityId{2},
      1002,
      9,
  };

  EXPECT_EQ(skill_system.ValidateAndConsume(request, 9.0F, 0.0F, &runtime),
            shared::ErrorCode::kOutOfRange);
}

TEST(SkillCastTest, CastConsumesMpOnSuccess) {
  SkillSystem skill_system;
  SkillRuntimeState runtime;
  runtime.current_mp = 30;

  skill_system.RegisterSkill(SkillDefinition{1003, 6.0F, 12U, 2.0F, 15});
  const shared::CastSkillRequest request{
      shared::EntityId{1},
      shared::EntityId{2},
      1003,
      11,
  };

  EXPECT_EQ(skill_system.ValidateAndConsume(request, 3.0F, 0.0F, &runtime),
            shared::ErrorCode::kOk);
  EXPECT_EQ(runtime.current_mp, 18U);
}

}  // namespace
}  // namespace server
