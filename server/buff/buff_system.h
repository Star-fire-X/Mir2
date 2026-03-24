#ifndef SERVER_BUFF_BUFF_SYSTEM_H_
#define SERVER_BUFF_BUFF_SYSTEM_H_

#include <cstdint>

#include "server/ecs/components.h"

namespace server {

class BuffSystem {
 public:
  void AddOrRefresh(ecs::BuffContainerComponent* buff_container,
                    std::uint32_t buff_id, float duration_seconds) const;

  void Tick(ecs::BuffContainerComponent* buff_container,
            float delta_seconds) const;

  [[nodiscard]] bool HasBuff(const ecs::BuffContainerComponent& buff_container,
                             std::uint32_t buff_id) const;
};

}  // namespace server

#endif  // SERVER_BUFF_BUFF_SYSTEM_H_
