#ifndef SERVER_CONFIG_CONFIG_MODEL_H_
#define SERVER_CONFIG_CONFIG_MODEL_H_

#include <cstdint>
#include <vector>

namespace server {

struct ItemTemplate {
  std::uint32_t id {};
};

struct SkillTemplate {
  std::uint32_t id {};
  int range {};
};

struct MonsterTemplate {
  std::uint32_t id {};
  std::uint32_t drop_item_id {};
  int move_speed {};
  std::uint32_t skill_id {};
};

struct MonsterSpawn {
  std::uint32_t monster_template_id {};
};

struct GameConfig {
  std::vector<ItemTemplate> item_templates;
  std::vector<SkillTemplate> skill_templates;
  std::vector<MonsterTemplate> monster_templates;
  std::vector<MonsterSpawn> monster_spawns;
};

}  // namespace server

#endif  // SERVER_CONFIG_CONFIG_MODEL_H_
