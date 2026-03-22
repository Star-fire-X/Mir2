#include <cstdint>

#include "gtest/gtest.h"
#include "server/config/config_validator.h"
#include "server/core/log/trace_context.h"

namespace server {
namespace {

GameConfig MakeBaseConfig() {
  GameConfig config;
  config.item_templates.push_back(ItemTemplate{.id = 1});
  config.skill_templates.push_back(SkillTemplate{.id = 1, .range = 3});
  config.monster_templates.push_back(MonsterTemplate{
      .id = 1,
      .drop_item_id = 1,
      .move_speed = 10,
      .skill_id = 1,
  });
  config.monster_spawns.push_back(MonsterSpawn{.monster_template_id = 1});
  return config;
}

TEST(ConfigValidatorTest, RejectsMissingMonsterTemplate) {
  GameConfig config = MakeBaseConfig();
  config.monster_spawns.clear();
  config.monster_spawns.push_back(MonsterSpawn{.monster_template_id = 99});

  EXPECT_FALSE(ConfigValidator::Validate(config));
}

TEST(ConfigValidatorTest, RejectsInvalidDropItemReference) {
  GameConfig config = MakeBaseConfig();
  config.monster_templates[0].drop_item_id = 2;

  EXPECT_FALSE(ConfigValidator::Validate(config));
}

TEST(ConfigValidatorTest, RejectsNegativeMoveSpeed) {
  GameConfig config = MakeBaseConfig();
  config.monster_templates[0].move_speed = -1;

  EXPECT_FALSE(ConfigValidator::Validate(config));
}

TEST(ConfigValidatorTest, RejectsInvalidSkillRange) {
  GameConfig config = MakeBaseConfig();
  config.skill_templates[0].range = 0;

  EXPECT_FALSE(ConfigValidator::Validate(config));
}

TEST(ConfigValidatorTest, TraceContextCarriesTraceAndEntityData) {
  TraceContext trace_context;
  trace_context.traceId = 7;
  trace_context.playerId = shared::PlayerId{11};
  trace_context.entityId = shared::EntityId{13};
  trace_context.clientSeq = 17;

  EXPECT_EQ(trace_context.traceId, 7u);
  EXPECT_EQ(trace_context.playerId, shared::PlayerId{11});
  EXPECT_EQ(trace_context.entityId, shared::EntityId{13});
  EXPECT_EQ(trace_context.clientSeq, 17u);
}

}  // namespace
}  // namespace server
