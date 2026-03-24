#ifndef SERVER_SCENE_MOVEMENT_SYSTEM_H_
#define SERVER_SCENE_MOVEMENT_SYSTEM_H_

#include <optional>
#include <vector>

#include "server/scene/scene.h"
#include "shared/protocol/scene_messages.h"

namespace server {

class MovementSystem {
 public:
  struct BlockedRect {
    float min_x = 0.0F;
    float min_y = 0.0F;
    float max_x = 0.0F;
    float max_y = 0.0F;
  };

  explicit MovementSystem(float max_speed_units_per_second = 6.0F,
                          float correction_threshold = 0.1F);

  void AddBlockedRect(BlockedRect blocked_rect);

  bool ApplyMove(Scene& scene, const shared::MoveRequest& move_request,
                 float delta_seconds,
                 std::optional<shared::MoveCorrection>* correction) const;

 private:
  [[nodiscard]] bool IsBlocked(const shared::ScenePosition& position) const;

  float max_speed_units_per_second_ = 6.0F;
  float correction_threshold_ = 0.1F;
  std::vector<BlockedRect> blocked_rects_;
};

}  // namespace server

#endif  // SERVER_SCENE_MOVEMENT_SYSTEM_H_
