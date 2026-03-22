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
  data.last_scene_snapshot.position = shared::ScenePosition{1.5F, 2.5F};
  data.version = 3;
  return data;
}

TEST(SaveServiceTest, SnapshotQueuedFromMainThread) {
  SaveService save_service;

  const PlayerSaveSnapshot snapshot =
      save_service.QueueSnapshotFromMainThread(MakeCharacterData());

  EXPECT_TRUE(save_service.HasQueuedSnapshot());
  EXPECT_EQ(snapshot.version, 1u);
  EXPECT_EQ(save_service.queued_snapshot().version, 1u);
  EXPECT_EQ(save_service.queued_snapshot().data.identity.player_id,
            shared::PlayerId{7});
}

TEST(SaveServiceTest, SuccessCallbackClearsDirtyFlag) {
  SaveService save_service;

  const PlayerSaveSnapshot snapshot =
      save_service.QueueSnapshotFromMainThread(MakeCharacterData());
  save_service.NotifySaveSuccess(snapshot.version);

  EXPECT_FALSE(save_service.IsDirty());
  EXPECT_FALSE(save_service.NeedsRetry());
}

TEST(SaveServiceTest, FailureCallbackKeepsRetryFlag) {
  SaveService save_service;

  const PlayerSaveSnapshot snapshot =
      save_service.QueueSnapshotFromMainThread(MakeCharacterData());
  save_service.NotifySaveFailure(snapshot.version);

  EXPECT_TRUE(save_service.IsDirty());
  EXPECT_TRUE(save_service.NeedsRetry());
}

}  // namespace
}  // namespace server
