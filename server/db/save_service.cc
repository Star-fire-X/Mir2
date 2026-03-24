#include "server/db/save_service.h"

namespace server {

PlayerSaveSnapshot SaveService::QueueSnapshotFromMainThread(
    const CharacterData& data) {
  const std::uint64_t version = next_version_;
  ++next_version_;
  queued_snapshot_ = PlayerSaveSnapshot{
      .data = data,
      .snapshot_version = version,
  };
  dirty_ = true;
  retry_ = false;
  return *queued_snapshot_;
}

bool SaveService::HasQueuedSnapshot() const {
  return queued_snapshot_.has_value();
}

const PlayerSaveSnapshot& SaveService::queued_snapshot() const {
  return *queued_snapshot_;
}

void SaveService::NotifySaveSuccess(std::uint64_t version) {
  if (!queued_snapshot_.has_value() ||
      queued_snapshot_->snapshot_version != version) {
    return;
  }

  queued_snapshot_.reset();
  dirty_ = false;
  retry_ = false;
}

void SaveService::NotifySaveFailure(std::uint64_t version) {
  if (!queued_snapshot_.has_value() ||
      queued_snapshot_->snapshot_version != version) {
    return;
  }

  dirty_ = true;
  retry_ = true;
}

bool SaveService::ShouldMarkDirty(SaveTrigger trigger) const {
  switch (trigger) {
    case SaveTrigger::kTimer:
      return false;
    case SaveTrigger::kInventoryChange:
    case SaveTrigger::kResourceChange:
    case SaveTrigger::kLogout:
    case SaveTrigger::kDisconnect:
      return true;
  }

  return false;
}

bool SaveService::ShouldQueueSnapshot(SaveTrigger trigger,
                                      bool player_dirty) const {
  switch (trigger) {
    case SaveTrigger::kInventoryChange:
    case SaveTrigger::kResourceChange:
      return false;
    case SaveTrigger::kTimer:
      return player_dirty && !HasQueuedSnapshot();
    case SaveTrigger::kLogout:
    case SaveTrigger::kDisconnect:
      return true;
  }

  return false;
}

bool SaveService::IsDirty() const { return dirty_; }

bool SaveService::NeedsRetry() const { return retry_; }

std::uint64_t SaveService::next_version() const { return next_version_; }

}  // namespace server
