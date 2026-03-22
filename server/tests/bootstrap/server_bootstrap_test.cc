#include "gtest/gtest.h"
#include "server/app/server_app.h"
#include "server/config/config_validator.h"
#include "server/config/config_manager.h"

namespace server {
namespace {

GameConfig BuildValidGameConfig() {
  GameConfig config;
  config.item_templates.push_back(ItemTemplate{101});
  config.skill_templates.push_back(SkillTemplate{301, 6});
  config.monster_templates.push_back(MonsterTemplate{201, 120, {301}});
  config.drop_tables.push_back(DropTableEntry{201, 101});
  return config;
}

TEST(ServerBootstrapTest, ConfigManagerStartsEmpty) {
  const ConfigManager config_manager;
  EXPECT_FALSE(config_manager.IsLoaded());
}

TEST(ServerBootstrapTest, ServerAppInitFailsOnInvalidConfig) {
  ServerApp server_app;

  GameConfig invalid_config = BuildValidGameConfig();
  invalid_config.monster_templates.clear();

  EXPECT_FALSE(server_app.Init(invalid_config));
}

}  // namespace
}  // namespace server
