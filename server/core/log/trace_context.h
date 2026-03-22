#ifndef SERVER_CORE_LOG_TRACE_CONTEXT_H_
#define SERVER_CORE_LOG_TRACE_CONTEXT_H_

#include <cstdint>

#include "shared/types/entity_id.h"
#include "shared/types/player_id.h"

namespace server {

struct TraceContext {
  std::uint64_t trace_id = 0;
  shared::PlayerId player_id{};
  shared::EntityId entity_id{};
  std::uint32_t client_seq = 0;
};

}  // namespace server

#endif  // SERVER_CORE_LOG_TRACE_CONTEXT_H_
