#include "server/config/config_validator.h"

#include <cstdint>

#include "gtest/gtest.h"

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

TEST(ConfigValidatorTest, RejectsDuplicateItemIds) {
  GameConfig config = MakeBaseConfig();
  config.item_templates.push_back(ItemTemplate{.id = 1});

  EXPECT_FALSE(ConfigValidator::Validate(config));
}

TEST(ConfigValidatorTest, RejectsZeroItemId) {
  GameConfig config = MakeBaseConfig();
  config.item_templates[0].id = 0;

  EXPECT_FALSE(ConfigValidator::Validate(config));
}

TEST(ConfigValidatorTest, RejectsDuplicateSkillTemplateIds) {
  GameConfig config = MakeBaseConfig();
  config.skill_templates.push_back(SkillTemplate{.id = 1, .range = 4});

  EXPECT_FALSE(ConfigValidator::Validate(config));
}

TEST(ConfigValidatorTest, RejectsZeroSkillId) {
  GameConfig config = MakeBaseConfig();
  config.skill_templates[0].id = 0;

  EXPECT_FALSE(ConfigValidator::Validate(config));
}

TEST(ConfigValidatorTest, RejectsDuplicateMonsterTemplateIds) {
  GameConfig config = MakeBaseConfig();
  config.monster_templates.push_back(MonsterTemplate{
      .id = 1,
      .drop_item_id = 1,
      .move_speed = 12,
      .skill_id = 1,
  });

  EXPECT_FALSE(ConfigValidator::Validate(config));
}

TEST(ConfigValidatorTest, RejectsZeroMonsterTemplateId) {
  GameConfig config = MakeBaseConfig();
  config.monster_templates[0].id = 0;

  EXPECT_FALSE(ConfigValidator::Validate(config));
}

TEST(ConfigValidatorTest, RejectsZeroMonsterSpawnTemplateId) {
  GameConfig config = MakeBaseConfig();
  config.monster_spawns[0].monster_template_id = 0;

  EXPECT_FALSE(ConfigValidator::Validate(config));
}

TEST(ConfigValidatorTest, RejectsZeroMonsterTemplateDropItemId) {
  GameConfig config = MakeBaseConfig();
  config.monster_templates[0].drop_item_id = 0;

  EXPECT_FALSE(ConfigValidator::Validate(config));
}

TEST(ConfigValidatorTest, RejectsZeroMonsterTemplateSkillId) {
  GameConfig config = MakeBaseConfig();
  config.monster_templates[0].skill_id = 0;

  EXPECT_FALSE(ConfigValidator::Validate(config));
}

}  // namespace
}  // namespace server
