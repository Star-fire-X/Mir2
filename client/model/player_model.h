#ifndef CLIENT_MODEL_PLAYER_MODEL_H_
#define CLIENT_MODEL_PLAYER_MODEL_H_

#include <cstdint>

#include "client/protocol/client_message.h"
#include "shared/protocol/scene_messages.h"
#include "shared/types/entity_id.h"

namespace client {

class PlayerModel {
 public:
  void ApplyEnterSceneSnapshot(const shared::EnterSceneSnapshot& snapshot) {
    controlled_entity_id_ = snapshot.controlled_entity_id;
    scene_id_ = snapshot.scene_id;
    position_ = snapshot.self_position;
  }

  void ApplySelfState(const protocol::SelfStateMessage& self_state) {
    if (controlled_entity_id_ == self_state.state.entity_id) {
      position_ = self_state.state.position;
    }
  }

  [[nodiscard]] shared::EntityId controlled_entity_id() const {
    return controlled_entity_id_;
  }

  [[nodiscard]] std::uint32_t scene_id() const { return scene_id_; }

  [[nodiscard]] const shared::ScenePosition& position() const {
    return position_;
  }

 private:
  shared::EntityId controlled_entity_id_{};
  std::uint32_t scene_id_ = 0;
  shared::ScenePosition position_{};
};

}  // namespace client

#endif  // CLIENT_MODEL_PLAYER_MODEL_H_
