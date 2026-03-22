#include "server/config/config_validator.h"

#include <cstdint>
#include <unordered_set>

namespace server {
namespace {

template <typename TEntry, typename TCollection>
bool HasUniqueNonZeroIds(const TCollection& collection,
                         std::unordered_set<std::uint32_t>& ids) {
  for (const auto& entry : collection) {
    if (entry.id == 0) {
      return false;
    }
    if (!ids.insert(entry.id).second) {
      return false;
    }
  }
  return true;
}

}  // namespace

bool ConfigValidator::Validate(const GameConfig& config) {
  std::unordered_set<std::uint32_t> item_ids;
  std::unordered_set<std::uint32_t> skill_ids;
  std::unordered_set<std::uint32_t> monster_template_ids;

  if (!HasUniqueNonZeroIds<ItemTemplate>(config.item_templates, item_ids)) {
    return false;
  }

  for (const auto& skill_template : config.skill_templates) {
    if (skill_template.id == 0 || skill_template.range <= 0) {
      return false;
    }
    if (!skill_ids.insert(skill_template.id).second) {
      return false;
    }
  }

  for (const auto& monster_template : config.monster_templates) {
    if (monster_template.id == 0 || monster_template.drop_item_id == 0 ||
        monster_template.skill_id == 0 || monster_template.move_speed < 0) {
      return false;
    }
    if (!monster_template_ids.insert(monster_template.id).second) {
      return false;
    }
    if (!item_ids.contains(monster_template.drop_item_id) ||
        !skill_ids.contains(monster_template.skill_id)) {
      return false;
    }
  }

  for (const auto& monster_spawn : config.monster_spawns) {
    if (monster_spawn.monster_template_id == 0 ||
        !monster_template_ids.contains(monster_spawn.monster_template_id)) {
      return false;
    }
  }

  return true;
}

}  // namespace server
