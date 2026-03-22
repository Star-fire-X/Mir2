#ifndef SHARED_PROTOCOL_SCENE_MESSAGES_H_
#define SHARED_PROTOCOL_SCENE_MESSAGES_H_

#include <cstdint>
#include <vector>

#include "shared/protocol/message_ids.h"
#include "shared/types/entity_id.h"
#include "shared/types/error_codes.h"
#include "shared/types/player_id.h"

namespace shared {

struct ScenePosition {
  float x = 0.0F;
  float y = 0.0F;
};

struct EnterSceneRequest {
  static constexpr MessageId kMessageId = MessageId::kEnterSceneRequest;
  PlayerId player_id;
  std::uint32_t scene_id = 0;
};

enum class VisibleEntityKind : std::uint8_t {
  kUnknown = 0,
  kPlayer = 1,
  kMonster = 2,
  kDrop = 3,
};

struct VisibleEntitySnapshot {
  EntityId entity_id;
  VisibleEntityKind kind = VisibleEntityKind::kUnknown;
  ScenePosition position;
};

struct EnterSceneSnapshot {
  static constexpr MessageId kMessageId = MessageId::kEnterSceneSnapshot;
  PlayerId player_id;
  EntityId controlled_entity_id;
  std::uint32_t scene_id = 0;
  ScenePosition self_position;
  std::vector<VisibleEntitySnapshot> visible_entities;
};

struct MoveRequest {
  static constexpr MessageId kMessageId = MessageId::kMoveRequest;
  EntityId entity_id;
  ScenePosition target_position;
  std::uint32_t client_seq = 0;
  std::uint64_t client_timestamp_ms = 0;
};

struct MoveCorrection {
  static constexpr MessageId kMessageId = MessageId::kMoveCorrection;
  EntityId entity_id;
  ScenePosition authoritative_position;
  std::uint32_t client_seq = 0;
};

struct PickupRequest {
  static constexpr MessageId kMessageId = MessageId::kPickupRequest;
  PlayerId player_id;
  EntityId drop_entity_id;
  std::uint32_t client_seq = 0;
};

struct PickupResult {
  static constexpr MessageId kMessageId = MessageId::kPickupResult;
  PlayerId player_id;
  EntityId drop_entity_id;
  ErrorCode error_code = ErrorCode::kOk;
  std::uint32_t client_seq = 0;
};

struct InventorySlotDelta {
  std::uint32_t slot_index = 0;
  std::uint32_t item_template_id = 0;
  std::uint32_t item_count = 0;
};

struct InventoryDelta {
  static constexpr MessageId kMessageId = MessageId::kInventoryDelta;
  PlayerId player_id;
  std::vector<InventorySlotDelta> slots;
};

}  // namespace shared

#endif  // SHARED_PROTOCOL_SCENE_MESSAGES_H_
