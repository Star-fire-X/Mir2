#include "server/app/server_app.h"

namespace {

server::GameConfig BuildStartupConfig() {
  server::GameConfig config;
  config.item_templates.push_back(server::ItemTemplate{1001});
  config.skill_templates.push_back(server::SkillTemplate{2001, 5});
  config.monster_templates.push_back(
      server::MonsterTemplate{3001, 120, {2001}});
  config.drop_tables.push_back(server::DropTableEntry{3001, 1001});
  return config;
}

}  // namespace

int main() {
  server::ServerApp server_app;
  return server_app.Init(BuildStartupConfig()) ? 0 : 1;
}
