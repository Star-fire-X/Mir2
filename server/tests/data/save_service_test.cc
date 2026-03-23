#include <cstdint>

#include "gtest/gtest.h"
#include "server/db/save_service.h"

namespace server {
namespace {

CharacterData MakeCharacterData() {
  CharacterData data;
  data.identity.player_id = shared::PlayerId{7};
  data.base_attributes.level = 12;
  data.inventory = Inventory{2};
  data.learned_skill_ids = {11, 22};
  data.last_scene_snapshot.scene_id = 8;
  data.last_scene_snapshot.position = WorldPosition{1.5F, 2.5F};
  data.version = 3;
  return data;
}

TEST(SaveServiceTest, SnapshotQueuedFromMainThread) {
  SaveService save_service;

  const PlayerSaveSnapshot snapshot =
      save_service.QueueSnapshotFromMainThread(MakeCharacterData());

  EXPECT_TRUE(save_service.HasQueuedSnapshot());
  EXPECT_EQ(snapshot.snapshot_version, 1u);
  EXPECT_EQ(save_service.queued_snapshot().snapshot_version, 1u);
  EXPECT_EQ(save_service.queued_snapshot().data.identity.player_id,
            shared::PlayerId{7});
}

TEST(SaveServiceTest, SuccessCallbackClearsDirtyFlag) {
  SaveService save_service;

  const PlayerSaveSnapshot snapshot =
      save_service.QueueSnapshotFromMainThread(MakeCharacterData());
  save_service.NotifySaveSuccess(snapshot.snapshot_version);

  EXPECT_FALSE(save_service.HasQueuedSnapshot());
  EXPECT_FALSE(save_service.IsDirty());
  EXPECT_FALSE(save_service.NeedsRetry());
}

TEST(SaveServiceTest, FailureCallbackKeepsRetryFlag) {
  SaveService save_service;

  const PlayerSaveSnapshot snapshot =
      save_service.QueueSnapshotFromMainThread(MakeCharacterData());
  save_service.NotifySaveFailure(snapshot.snapshot_version);

  ASSERT_TRUE(save_service.HasQueuedSnapshot());
  EXPECT_EQ(save_service.queued_snapshot().snapshot_version,
            snapshot.snapshot_version);
  EXPECT_TRUE(save_service.IsDirty());
  EXPECT_TRUE(save_service.NeedsRetry());
}

TEST(SaveServiceTest, RequeueAdvancesVersionAndIgnoresOlderSuccessCallback) {
  SaveService save_service;

  const PlayerSaveSnapshot first_snapshot =
      save_service.QueueSnapshotFromMainThread(MakeCharacterData());
  CharacterData updated_data = MakeCharacterData();
  updated_data.base_attributes.level = 13;
  const PlayerSaveSnapshot second_snapshot =
      save_service.QueueSnapshotFromMainThread(updated_data);

  save_service.NotifySaveSuccess(first_snapshot.snapshot_version);

  EXPECT_EQ(first_snapshot.snapshot_version, 1u);
  EXPECT_EQ(second_snapshot.snapshot_version, 2u);
  EXPECT_TRUE(save_service.HasQueuedSnapshot());
  EXPECT_EQ(save_service.queued_snapshot().snapshot_version,
            second_snapshot.snapshot_version);
  EXPECT_EQ(save_service.queued_snapshot().data.base_attributes.level, 13u);
  EXPECT_TRUE(save_service.IsDirty());
  EXPECT_FALSE(save_service.NeedsRetry());
}

TEST(SaveServiceTest, RequeueIgnoresOlderFailureCallback) {
  SaveService save_service;

  const PlayerSaveSnapshot first_snapshot =
      save_service.QueueSnapshotFromMainThread(MakeCharacterData());
  CharacterData updated_data = MakeCharacterData();
  updated_data.base_attributes.level = 14;
  const PlayerSaveSnapshot second_snapshot =
      save_service.QueueSnapshotFromMainThread(updated_data);

  save_service.NotifySaveFailure(first_snapshot.snapshot_version);

  ASSERT_TRUE(save_service.HasQueuedSnapshot());
  EXPECT_EQ(save_service.queued_snapshot().snapshot_version,
            second_snapshot.snapshot_version);
  EXPECT_EQ(save_service.queued_snapshot().data.base_attributes.level, 14u);
  EXPECT_TRUE(save_service.IsDirty());
  EXPECT_FALSE(save_service.NeedsRetry());
}

}  // namespace
}  // namespace server
