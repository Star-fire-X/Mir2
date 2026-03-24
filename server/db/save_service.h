#ifndef SERVER_DB_SAVE_SERVICE_H_
#define SERVER_DB_SAVE_SERVICE_H_

#include <cstdint>
#include <optional>

#include "server/db/player_save_snapshot.h"

namespace server {

enum class SaveTrigger : std::uint8_t {
  kTimer = 0,
  kInventoryChange = 1,
  kResourceChange = 2,
  kLogout = 3,
  kDisconnect = 4,
};

class SaveService {
 public:
  SaveService() = default;

  PlayerSaveSnapshot QueueSnapshotFromMainThread(const CharacterData& data);
  bool HasQueuedSnapshot() const;
  const PlayerSaveSnapshot& queued_snapshot() const;

  void NotifySaveSuccess(std::uint64_t version);
  void NotifySaveFailure(std::uint64_t version);

  bool ShouldMarkDirty(SaveTrigger trigger) const;
  bool ShouldQueueSnapshot(SaveTrigger trigger, bool player_dirty) const;
  bool IsDirty() const;
  bool NeedsRetry() const;
  std::uint64_t next_version() const;

 private:
  std::optional<PlayerSaveSnapshot> queued_snapshot_;
  bool dirty_ = false;
  bool retry_ = false;
  std::uint64_t next_version_ = 1;
};

}  // namespace server

#endif  // SERVER_DB_SAVE_SERVICE_H_
