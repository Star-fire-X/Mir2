#include "server/app/server_app.h"

#include <utility>

#include "server/config/config_validator.h"

namespace server {
namespace {

GameConfig BuildDefaultConfig() {
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

}  // namespace

ConfigManager ServerApp::BuildDefaultConfigManager() {
  return ConfigManager{BuildDefaultConfig()};
}

ServerApp::ServerApp() : config_manager_(BuildDefaultConfigManager()) {}

ServerApp::ServerApp(ConfigManager config_manager)
    : config_manager_(std::move(config_manager)) {}

bool ServerApp::Init() {
  if (!config_manager_.IsLoaded()) {
    return false;
  }
  return ConfigValidator::Validate(config_manager_.GetConfig());
}

}  // namespace server
