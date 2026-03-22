#include <array>
#include <concepts>
#include <cstdint>
#include <set>
#include <sstream>

#include "gtest/gtest.h"
#include "shared/protocol/message_ids.h"
#include "shared/protocol/scene_messages.h"
#include "shared/types/entity_id.h"

namespace server {
namespace {

namespace shared_contract {

template <typename T>
concept HasControlledEntityId = requires(T value) {
  { value.controlled_entity_id } -> std::same_as<shared::EntityId&>;
};

template <typename T>
concept HasVisibleEntityKind = requires(T value) { value.kind; };

}  // namespace shared_contract

TEST(SharedContractTest, EntityIdIsComparableAndPrintable) {
  const shared::EntityId low{1};
  const shared::EntityId high{2};
  EXPECT_LT(low, high);
  EXPECT_EQ(low, shared::EntityId{1});

  std::ostringstream stream;
  stream << shared::EntityId{42};
  EXPECT_EQ(stream.str(), "42");
}

TEST(SharedContractTest, MessageIdsAreUnique) {
  constexpr std::array<shared::MessageId, 11> kMessageIds = {
      shared::MessageId::kLoginRequest,
      shared::MessageId::kLoginResponse,
      shared::MessageId::kEnterSceneRequest,
      shared::MessageId::kEnterSceneSnapshot,
      shared::MessageId::kMoveRequest,
      shared::MessageId::kMoveCorrection,
      shared::MessageId::kPickupRequest,
      shared::MessageId::kPickupResult,
      shared::MessageId::kInventoryDelta,
      shared::MessageId::kCastSkillRequest,
      shared::MessageId::kCastSkillResult,
  };

  std::set<std::uint16_t> unique_ids;
  for (const shared::MessageId message_id : kMessageIds) {
    unique_ids.insert(static_cast<std::uint16_t>(message_id));
  }

  EXPECT_EQ(unique_ids.size(), kMessageIds.size());
}

TEST(SharedContractTest, EnterSceneSnapshotExposesControlledEntityId) {
  EXPECT_TRUE(
      (shared_contract::HasControlledEntityId<shared::EnterSceneSnapshot>));
}

TEST(SharedContractTest, VisibleEntitySnapshotCarriesEntityType) {
  EXPECT_TRUE(
      (shared_contract::HasVisibleEntityKind<shared::VisibleEntitySnapshot>));
}

}  // namespace
}  // namespace server
