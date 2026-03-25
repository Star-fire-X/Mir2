#include "server/app/server_app.h"
#include "server/config/config_manager.h"
#include "server/runtime/server_runtime.h"

#include <string_view>
#include <thread>  // NOLINT(build/c++11)

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

int main(int argc, char** argv) {
  server::ConfigManager config_manager;
  config_manager.Load(BuildMinimalValidConfig());
  if (argc > 1 && std::string_view(argv[1]) == "--smoke-check") {
    server::ServerApp app(config_manager);
    return app.Init() ? 0 : 1;
  }

  server::ServerRuntime runtime(config_manager);
  if (!runtime.Start()) {
    return 1;
  }

  while (runtime.running()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  return 0;
}
