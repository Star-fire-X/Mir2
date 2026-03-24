#include "client/controller/player_controller.h"

#include <utility>

namespace client {

PlayerController::PlayerController(MoveRequestSink move_request_sink)
    : move_request_sink_(std::move(move_request_sink)) {}

void PlayerController::SetMoveRequestSink(MoveRequestSink move_request_sink) {
  move_request_sink_ = std::move(move_request_sink);
}

void PlayerController::HandleMoveInput(
    shared::EntityId entity_id, const shared::ScenePosition& target_position,
    std::uint32_t client_seq, std::uint64_t client_timestamp_ms) const {
  if (!move_request_sink_) {
    return;
  }

  move_request_sink_(shared::MoveRequest{
      entity_id,
      target_position,
      client_seq,
      client_timestamp_ms,
  });
}

}  // namespace client
