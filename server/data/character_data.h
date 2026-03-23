#ifndef SERVER_DATA_CHARACTER_DATA_H_
#define SERVER_DATA_CHARACTER_DATA_H_

#include <cstdint>
#include <string>
#include <vector>

#include "server/data/inventory.h"
#include "shared/types/player_id.h"

namespace server {

struct CharacterIdentity {
  shared::PlayerId player_id{};
  std::string character_name;
};

struct BaseAttributes {
  std::uint32_t level{};
  std::uint32_t strength{};
  std::uint32_t agility{};
  std::uint32_t vitality{};
  std::uint32_t intelligence{};
};

struct WorldPosition {
  float x = 0.0F;
  float y = 0.0F;
};

struct LastSceneSnapshot {
  std::uint32_t scene_id{};
  WorldPosition position{};
};

struct CharacterData {
  CharacterIdentity identity{};
  BaseAttributes base_attributes{};
  Inventory inventory{};
  std::vector<std::uint32_t> learned_skill_ids;
  LastSceneSnapshot last_scene_snapshot{};
  std::uint64_t version{};
};

}  // namespace server

#endif  // SERVER_DATA_CHARACTER_DATA_H_
