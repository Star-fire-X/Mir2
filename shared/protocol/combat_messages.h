#ifndef SHARED_PROTOCOL_COMBAT_MESSAGES_H_
#define SHARED_PROTOCOL_COMBAT_MESSAGES_H_

#include <cstdint>

#include "shared/protocol/message_ids.h"
#include "shared/types/entity_id.h"
#include "shared/types/error_codes.h"

namespace shared {

struct CastSkillRequest {
  static constexpr MessageId kMessageId = MessageId::kCastSkillRequest;
  EntityId caster_entity_id;
  EntityId target_entity_id;
  std::uint32_t skill_id = 0;
  std::uint32_t client_seq = 0;
};

struct CastSkillResult {
  static constexpr MessageId kMessageId = MessageId::kCastSkillResult;
  EntityId caster_entity_id;
  EntityId target_entity_id;
  std::uint32_t skill_id = 0;
  ErrorCode error_code = ErrorCode::kOk;
  std::int32_t hp_delta = 0;
  std::uint32_t client_seq = 0;
};

}  // namespace shared

#endif  // SHARED_PROTOCOL_COMBAT_MESSAGES_H_
