#ifndef SERVER_DB_PLAYER_SAVE_SNAPSHOT_H_
#define SERVER_DB_PLAYER_SAVE_SNAPSHOT_H_

#include <cstdint>

#include "server/data/character_data.h"

namespace server {

struct PlayerSaveSnapshot {
  CharacterData data;
  std::uint64_t snapshot_version{};
};

}  // namespace server

#endif  // SERVER_DB_PLAYER_SAVE_SNAPSHOT_H_
