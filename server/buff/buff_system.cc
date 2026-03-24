#include "server/buff/buff_system.h"

#include <algorithm>
#include <cstdint>

namespace server {

void BuffSystem::AddOrRefresh(ecs::BuffContainerComponent* buff_container,
                              std::uint32_t buff_id,
                              float duration_seconds) const {
  if (buff_container == nullptr || buff_id == 0 || duration_seconds <= 0.0F) {
    return;
  }

  if (std::find(buff_container->buff_ids.begin(),
                buff_container->buff_ids.end(),
                buff_id) == buff_container->buff_ids.end()) {
    buff_container->buff_ids.push_back(buff_id);
  }
  buff_container->remaining_seconds_by_buff_id[buff_id] = duration_seconds;
}

void BuffSystem::Tick(ecs::BuffContainerComponent* buff_container,
                      float delta_seconds) const {
  if (buff_container == nullptr || delta_seconds <= 0.0F) {
    return;
  }

  for (auto it = buff_container->remaining_seconds_by_buff_id.begin();
       it != buff_container->remaining_seconds_by_buff_id.end();) {
    it->second -= delta_seconds;
    if (it->second > 0.0F) {
      ++it;
      continue;
    }

    const std::uint32_t expired_buff_id = it->first;
    it = buff_container->remaining_seconds_by_buff_id.erase(it);
    buff_container->buff_ids.erase(
        std::remove(buff_container->buff_ids.begin(),
                    buff_container->buff_ids.end(), expired_buff_id),
        buff_container->buff_ids.end());
  }
}

bool BuffSystem::HasBuff(const ecs::BuffContainerComponent& buff_container,
                         std::uint32_t buff_id) const {
  return std::find(buff_container.buff_ids.begin(),
                   buff_container.buff_ids.end(),
                   buff_id) != buff_container.buff_ids.end();
}

}  // namespace server
