#include "server/app/server_app.h"
#include "server/config/config_manager.h"

namespace {

server::GameConfig BuildMinimalValidConfig() {
  server::GameConfig config;
  config.item_templates.push_back(server::ItemTemplate{.id = 1});
  config.skill_templates.push_back(server::SkillTemplate{.id = 1, .range = 1});
  config.monster_templates.push_back(server::MonsterTemplate{
      .id = 1,
      .drop_item_id = 1,
      .move_speed = 1,
      .skill_id = 1,
  });
  config.monster_spawns.push_back(
      server::MonsterSpawn{.monster_template_id = 1});
  return config;
}

}  // namespace

int main() {
  server::ConfigManager config_manager;
  config_manager.Load(BuildMinimalValidConfig());
  server::ServerApp app(config_manager);
  return app.Init() ? 0 : 1;
}
