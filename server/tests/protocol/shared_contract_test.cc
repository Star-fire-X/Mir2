#include <array>
#include <cstdint>
#include <set>
#include <sstream>
#include <type_traits>
#include <utility>

#include "gtest/gtest.h"
#include "shared/protocol/message_ids.h"
#include "shared/protocol/scene_messages.h"
#include "shared/types/entity_id.h"

template <typename T, typename = void>
struct SharedContractHasControlledEntityId : std::false_type {};

template <typename T>
using SharedContractControlledEntityIdMember =
    decltype(std::declval<T*>()->controlled_entity_id);

template <typename T>
struct SharedContractHasControlledEntityId<
    T, std::void_t<SharedContractControlledEntityIdMember<T>>>
    : std::true_type {};

template <typename T, typename = void>
struct SharedContractHasVisibleEntityKind : std::false_type {};

template <typename T>
using SharedContractVisibleEntityKindMember =
    decltype(std::declval<T*>()->kind);

template <typename T>
struct SharedContractHasVisibleEntityKind<
    T, std::void_t<SharedContractVisibleEntityKindMember<T>>> : std::true_type {
};

namespace server {
namespace {

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
      (SharedContractHasControlledEntityId<shared::EnterSceneSnapshot>::value));
  EXPECT_TRUE(
      (std::is_same_v<
          SharedContractControlledEntityIdMember<shared::EnterSceneSnapshot>,
          shared::EntityId>));
}

TEST(SharedContractTest, VisibleEntitySnapshotCarriesEntityType) {
  EXPECT_TRUE((SharedContractHasVisibleEntityKind<
               shared::VisibleEntitySnapshot>::value));
}

}  // namespace
}  // namespace server
