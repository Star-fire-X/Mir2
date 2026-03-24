#ifndef CLIENT_DEBUG_DEV_PANEL_H_
#define CLIENT_DEBUG_DEV_PANEL_H_

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "shared/protocol/scene_messages.h"
#include "shared/types/entity_id.h"

namespace client {

struct DevPanelSnapshot {
  std::uint32_t scene_id = 0;
  std::uint32_t ping_ms = 0;
  std::size_t entity_count = 0;
  shared::EntityId target_entity_id{};
  shared::ScenePosition player_position{};
  std::vector<std::string> recent_protocol_summaries;
};

class DevPanel {
 public:
  void Update(DevPanelSnapshot snapshot) { snapshot_ = std::move(snapshot); }

  [[nodiscard]] const DevPanelSnapshot& snapshot() const { return snapshot_; }

 private:
  DevPanelSnapshot snapshot_;
};

}  // namespace client

#endif  // CLIENT_DEBUG_DEV_PANEL_H_
