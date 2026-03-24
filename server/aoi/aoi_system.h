#ifndef SERVER_AOI_AOI_SYSTEM_H_
#define SERVER_AOI_AOI_SYSTEM_H_

#include <cstdint>
#include <vector>

#include "server/scene/scene.h"
#include "shared/protocol/scene_messages.h"
#include "shared/types/entity_id.h"

namespace server {

struct AoiDiff {
  std::vector<shared::VisibleEntitySnapshot> entered;
  std::vector<shared::EntityId> left;
};

class AoiSystem {
 public:
  explicit AoiSystem(float visible_radius = 10.0F);

  shared::EnterSceneSnapshot BuildEnterSceneSnapshot(
      shared::PlayerId player_id, shared::EntityId self_entity_id,
      std::uint32_t scene_id, const Scene& scene) const;

  AoiDiff ComputeEnterLeave(shared::EntityId observer_entity_id,
                            const shared::ScenePosition& old_position,
                            const shared::ScenePosition& new_position,
                            const Scene& scene) const;

 private:
  [[nodiscard]] bool IsVisible(const shared::ScenePosition& from,
                               const shared::ScenePosition& to) const;

  float visible_radius_ = 10.0F;
};

}  // namespace server

#endif  // SERVER_AOI_AOI_SYSTEM_H_
