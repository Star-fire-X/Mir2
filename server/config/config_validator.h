#ifndef SERVER_CONFIG_CONFIG_VALIDATOR_H_
#define SERVER_CONFIG_CONFIG_VALIDATOR_H_

#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace server {

struct SkillTemplate {
  int id = 0;
  int range = 0;
};

struct MonsterTemplate {
  int id = 0;
  int move_speed = 0;
  std::vector<int> skill_ids;
};

struct ItemTemplate {
  int id = 0;
};

struct DropTableEntry {
  int monster_id = 0;
  int item_id = 0;
};

struct GameConfig {
  std::vector<MonsterTemplate> monster_templates;
  std::vector<SkillTemplate> skill_templates;
  std::vector<ItemTemplate> item_templates;
  std::vector<DropTableEntry> drop_tables;
};

class ConfigValidator {
 public:
  static std::vector<std::string> Validate(const GameConfig& config) {
    std::vector<std::string> errors;

    if (config.monster_templates.empty()) {
      errors.emplace_back("missing monster template");
    }
    if (config.skill_templates.empty()) {
      errors.emplace_back("missing skill template");
    }
    if (config.item_templates.empty()) {
      errors.emplace_back("missing item template");
    }

    std::set<int> monster_ids;
    std::set<int> skill_ids;
    std::set<int> item_ids;

    for (const MonsterTemplate& monster : config.monster_templates) {
      if (!monster_ids.insert(monster.id).second) {
        errors.emplace_back("duplicate monster id");
      }
      if (monster.move_speed < 0) {
        errors.emplace_back("negative move speed");
      }
    }

    for (const SkillTemplate& skill : config.skill_templates) {
      if (!skill_ids.insert(skill.id).second) {
        errors.emplace_back("duplicate skill id");
      }
      if (skill.range <= 0) {
        errors.emplace_back("invalid skill range");
      }
    }

    for (const ItemTemplate& item : config.item_templates) {
      if (!item_ids.insert(item.id).second) {
        errors.emplace_back("duplicate item id");
      }
    }

    for (const MonsterTemplate& monster : config.monster_templates) {
      for (const int skill_id : monster.skill_ids) {
        if (!skill_ids.contains(skill_id)) {
          errors.emplace_back("monster references missing skill");
        }
      }
    }

    for (const DropTableEntry& drop : config.drop_tables) {
      if (!monster_ids.contains(drop.monster_id)) {
        errors.emplace_back("drop references missing monster");
      }
      if (!item_ids.contains(drop.item_id)) {
        errors.emplace_back("drop references missing item");
      }
    }

    return errors;
  }
};

}  // namespace server

#endif  // SERVER_CONFIG_CONFIG_VALIDATOR_H_
