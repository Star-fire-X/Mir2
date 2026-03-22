#include "server/db/save_service.h"

namespace server {

PlayerSaveSnapshot SaveService::QueueSnapshotFromMainThread(
    const CharacterData& data) {
  queued_snapshot_ = PlayerSaveSnapshot{
      .data = data,
      .version = next_version_,
  };
  dirty_ = true;
  retry_ = false;
  return *queued_snapshot_;
}

bool SaveService::HasQueuedSnapshot() const { return queued_snapshot_.has_value(); }

const PlayerSaveSnapshot& SaveService::queued_snapshot() const {
  return *queued_snapshot_;
}

void SaveService::NotifySaveSuccess(std::uint64_t version) {
  if (!queued_snapshot_.has_value() || queued_snapshot_->version != version) {
    return;
  }

  queued_snapshot_.reset();
  dirty_ = false;
  retry_ = false;
  next_version_ = version + 1;
}

void SaveService::NotifySaveFailure(std::uint64_t version) {
  if (!queued_snapshot_.has_value() || queued_snapshot_->version != version) {
    return;
  }

  dirty_ = true;
  retry_ = true;
}

bool SaveService::IsDirty() const { return dirty_; }

bool SaveService::NeedsRetry() const { return retry_; }

std::uint64_t SaveService::next_version() const { return next_version_; }

}  // namespace server
