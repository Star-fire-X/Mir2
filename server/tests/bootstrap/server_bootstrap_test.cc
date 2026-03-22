#include "gtest/gtest.h"
#include "server/app/server_app.h"
#include "server/config/config_manager.h"
#include "server/config/config_validator.h"

namespace server {
namespace {

TEST(ServerBootstrapTest, ConfigManagerStartsEmpty) {
  const ConfigManager config_manager;
  EXPECT_FALSE(config_manager.IsLoaded());
}

TEST(ServerBootstrapTest, ServerAppInitFailsWhenConfigValidationFails) {
  ConfigManager config_manager;
  GameConfig config;
  config.monster_spawns.push_back(MonsterSpawn{.monster_template_id = 42});
  config_manager.SetConfig(config);

  ServerApp app{config_manager};
  EXPECT_FALSE(app.Init());
}

}  // namespace
}  // namespace server
