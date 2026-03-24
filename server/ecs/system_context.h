#ifndef SERVER_ECS_SYSTEM_CONTEXT_H_
#define SERVER_ECS_SYSTEM_CONTEXT_H_

#include <cstdint>

namespace server::ecs {

struct SystemContext {
  std::uint64_t tick_index = 0;
  std::uint64_t now_ms = 0;
};

}  // namespace server::ecs

#endif  // SERVER_ECS_SYSTEM_CONTEXT_H_
