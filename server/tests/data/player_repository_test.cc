#include "server/db/player_repository.h"

#include <cstdint>

#include "gtest/gtest.h"

namespace server {
namespace {

CharacterData MakeCharacterData(std::uint32_t level) {
  CharacterData data;
  data.identity.player_id = shared::PlayerId{7};
  data.base_attributes.level = level;
  data.inventory = Inventory{2};
  data.learned_skill_ids = {11, 22};
  data.last_scene_snapshot.scene_id = 8;
  data.last_scene_snapshot.position = WorldPosition{1.5F, 2.5F};
  data.version = 3;
  return data;
}

TEST(PlayerRepositoryTest, RejectsStaleSnapshotAfterNewerSave) {
  PlayerRepository repository;

  const PlayerSaveSnapshot newer_snapshot{
      .data = MakeCharacterData(13),
      .snapshot_version = 2,
  };
  const PlayerSaveSnapshot older_snapshot{
      .data = MakeCharacterData(12),
      .snapshot_version = 1,
  };

  EXPECT_TRUE(repository.Save(newer_snapshot));
  EXPECT_FALSE(repository.Save(older_snapshot));

  ASSERT_TRUE(repository.last_saved_snapshot().has_value());
  EXPECT_EQ(repository.last_saved_snapshot()->snapshot_version, 2u);
  EXPECT_EQ(repository.last_saved_snapshot()->data.base_attributes.level, 13u);
}

}  // namespace
}  // namespace server
