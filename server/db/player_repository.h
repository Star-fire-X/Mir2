#ifndef SERVER_DB_PLAYER_REPOSITORY_H_
#define SERVER_DB_PLAYER_REPOSITORY_H_

#include <optional>

#include "server/data/character_data.h"

namespace server {

class PlayerRepository {
 public:
  PlayerRepository() = default;

  bool Save(const PlayerSaveSnapshot& snapshot);
  const std::optional<PlayerSaveSnapshot>& last_saved_snapshot() const;

 private:
  std::optional<PlayerSaveSnapshot> last_saved_snapshot_;
};

}  // namespace server

#endif  // SERVER_DB_PLAYER_REPOSITORY_H_
