#include "server/db/player_repository.h"

namespace server {

bool PlayerRepository::Save(const PlayerSaveSnapshot& snapshot) {
  last_saved_snapshot_ = snapshot;
  return true;
}

const std::optional<PlayerSaveSnapshot>& PlayerRepository::last_saved_snapshot()
    const {
  return last_saved_snapshot_;
}

}  // namespace server
