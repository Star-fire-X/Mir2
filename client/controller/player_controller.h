#ifndef CLIENT_CONTROLLER_PLAYER_CONTROLLER_H_
#define CLIENT_CONTROLLER_PLAYER_CONTROLLER_H_

#include <cstdint>
#include <functional>

#include "shared/protocol/scene_messages.h"
#include "shared/types/entity_id.h"

namespace client {

class PlayerController {
 public:
  using MoveRequestSink = std::function<void(const shared::MoveRequest&)>;

  explicit PlayerController(MoveRequestSink move_request_sink = {});

  void SetMoveRequestSink(MoveRequestSink move_request_sink);

  void HandleMoveInput(shared::EntityId entity_id,
                       const shared::ScenePosition& target_position,
                       std::uint32_t client_seq,
                       std::uint64_t client_timestamp_ms) const;

 private:
  MoveRequestSink move_request_sink_;
};

}  // namespace client

#endif  // CLIENT_CONTROLLER_PLAYER_CONTROLLER_H_
