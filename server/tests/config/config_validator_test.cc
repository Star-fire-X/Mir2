#include <string_view>
#include <vector>

#include "gtest/gtest.h"
#include "server/config/config_validator.h"

namespace server {
namespace {

GameConfig BuildValidGameConfig() {
  GameConfig config;
  config.item_templates.push_back(ItemTemplate{1001});
  config.skill_templates.push_back(SkillTemplate{2001, 5});
  config.monster_templates.push_back(MonsterTemplate{3001, 120, {2001}});
  config.drop_tables.push_back(DropTableEntry{3001, 1001});
  return config;
}

bool ContainsMessage(const std::vector<std::string>& errors,
                     std::string_view needle) {
  for (const std::string& error : errors) {
    if (error.find(needle) != std::string::npos) {
      return true;
    }
  }
  return false;
}

TEST(ConfigValidatorTest, RejectsMissingMonsterTemplate) {
  GameConfig config = BuildValidGameConfig();
  config.monster_templates.clear();

  const std::vector<std::string> errors = ConfigValidator::Validate(config);

  EXPECT_FALSE(errors.empty());
  EXPECT_TRUE(ContainsMessage(errors, "monster"));
}

TEST(ConfigValidatorTest, RejectsDropItemReferenceToMissingItem) {
  GameConfig config = BuildValidGameConfig();
  config.drop_tables[0].item_id = 9999;

  const std::vector<std::string> errors = ConfigValidator::Validate(config);

  EXPECT_FALSE(errors.empty());
  EXPECT_TRUE(ContainsMessage(errors, "drop"));
}

TEST(ConfigValidatorTest, RejectsNegativeMoveSpeed) {
  GameConfig config = BuildValidGameConfig();
  config.monster_templates[0].move_speed = -1;

  const std::vector<std::string> errors = ConfigValidator::Validate(config);

  EXPECT_FALSE(errors.empty());
  EXPECT_TRUE(ContainsMessage(errors, "move speed"));
}

TEST(ConfigValidatorTest, RejectsInvalidSkillRange) {
  GameConfig config = BuildValidGameConfig();
  config.skill_templates[0].range = -3;

  const std::vector<std::string> errors = ConfigValidator::Validate(config);

  EXPECT_FALSE(errors.empty());
  EXPECT_TRUE(ContainsMessage(errors, "skill range"));
}

}  // namespace
}  // namespace server
