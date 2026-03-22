#include "gtest/gtest.h"
#include "server/app/server_app.h"
#include "server/config/config_manager.h"

namespace server {
namespace {

GameConfig MakeValidConfig() {
  GameConfig config;
  config.item_templates.push_back(ItemTemplate{.id = 1});
  config.skill_templates.push_back(SkillTemplate{.id = 1, .range = 3});
  config.monster_templates.push_back(MonsterTemplate{
      .id = 1,
      .drop_item_id = 1,
      .move_speed = 1,
      .skill_id = 1,
  });
  config.monster_spawns.push_back(MonsterSpawn{.monster_template_id = 1});
  return config;
}

TEST(ServerBootstrapTest, ConfigManagerStartsEmpty) {
  const ConfigManager config_manager;
  EXPECT_FALSE(config_manager.IsLoaded());
}

TEST(ServerBootstrapTest, ConfigManagerCanBeExplicitlyLoaded) {
  ConfigManager config_manager;
  config_manager.Load(MakeValidConfig());

  EXPECT_TRUE(config_manager.IsLoaded());
}

TEST(ServerBootstrapTest, ServerAppInitFailsWhenLoadedConfigIsInvalid) {
  ConfigManager config_manager;
  GameConfig config;
  config.monster_spawns.push_back(MonsterSpawn{.monster_template_id = 42});
  config_manager.Load(config);

  ServerApp app{config_manager};
  EXPECT_FALSE(app.Init());
}

TEST(ServerBootstrapTest, ServerAppInitSucceedsWhenLoadedConfigIsValid) {
  ConfigManager config_manager;
  config_manager.Load(MakeValidConfig());

  ServerApp app{config_manager};
  EXPECT_TRUE(app.Init());
}

}  // namespace
}  // namespace server
