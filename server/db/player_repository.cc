#include "server/db/player_repository.h"

namespace server {

bool PlayerRepository::Save(const PlayerSaveSnapshot& snapshot) {
  last_saved_snapshot_ = snapshot;
  return true;
}

using Snapshot = std::optional<PlayerSaveSnapshot>;

const Snapshot& PlayerRepository::last_saved_snapshot() const {
  return last_saved_snapshot_;
}

}  // namespace server
