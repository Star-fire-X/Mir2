#ifndef CLIENT_VIEW_ENTITY_VIEW_H_
#define CLIENT_VIEW_ENTITY_VIEW_H_

#include <algorithm>
#include <cstdint>

#include "shared/protocol/scene_messages.h"
#include "shared/types/entity_id.h"

namespace client {

enum class EntityViewKind : std::uint8_t {
  kUnknown = 0,
  kPlayer = 1,
  kMonster = 2,
  kDrop = 3,
};

class EntityView {
 public:
  EntityView(shared::EntityId entity_id, const shared::ScenePosition& position,
             EntityViewKind kind)
      : entity_id_(entity_id),
        position_(position),
        render_position_(position),
        kind_(kind) {}
  virtual ~EntityView() = default;

  virtual void ApplyDelta(const shared::ScenePosition& position) {
    position_ = position;
  }

  virtual void UpdateInterpolation(float alpha) {
    const float blend = std::clamp(alpha, 0.0F, 1.0F);
    render_position_.x =
        (position_.x * blend) + (render_position_.x * (1.0F - blend));
    render_position_.y =
        (position_.y * blend) + (render_position_.y * (1.0F - blend));
  }

  [[nodiscard]] EntityViewKind kind() const { return kind_; }
  [[nodiscard]] shared::EntityId entity_id() const { return entity_id_; }
  [[nodiscard]] const shared::ScenePosition& position() const {
    return position_;
  }
  [[nodiscard]] const shared::ScenePosition& render_position() const {
    return render_position_;
  }

 private:
  shared::EntityId entity_id_{};
  shared::ScenePosition position_{};
  shared::ScenePosition render_position_{};
  EntityViewKind kind_ = EntityViewKind::kUnknown;
};

}  // namespace client

#endif  // CLIENT_VIEW_ENTITY_VIEW_H_
