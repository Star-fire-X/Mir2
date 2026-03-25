#ifndef SERVER_CORE_LOG_TRACE_CONTEXT_H_
#define SERVER_CORE_LOG_TRACE_CONTEXT_H_

#include <cstdint>

#include "shared/protocol/message_ids.h"
#include "shared/types/entity_id.h"
#include "shared/types/player_id.h"

namespace server {

struct TraceContext {
  std::uint64_t traceId{};
  shared::PlayerId playerId{};
  shared::EntityId entityId{};
  std::uint32_t clientSeq{};
  shared::MessageId messageId{};
  auto operator<=>(const TraceContext&) const = default;
};

}  // namespace server

#endif  // SERVER_CORE_LOG_TRACE_CONTEXT_H_
