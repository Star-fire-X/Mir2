#include "server/player/player.h"

#include <utility>

namespace server {

Player::Player(CharacterData data) : data_(std::move(data)) {}

const CharacterData& Player::data() const { return data_; }

CharacterData& Player::mutable_data() { return data_; }

Session* Player::session() const { return session_; }

bool Player::dirty() const { return dirty_; }

const std::optional<shared::EntityId>& Player::controlled_entity_id() const {
  return controlled_entity_id_;
}

std::uint32_t Player::current_scene_id() const { return current_scene_id_; }

void Player::BindSession(Session* session) { session_ = session; }

void Player::SetControlledEntity(shared::EntityId entity_id,
                                 std::uint32_t scene_id) {
  controlled_entity_id_ = entity_id;
  current_scene_id_ = scene_id;
}

void Player::SubmitMove(Scene& scene, const shared::MoveRequest& move_request) {
  scene.Enqueue(SceneCommand{
      SceneCommandType::kMove,
      move_request,
  });
}

void Player::MarkDirty() { dirty_ = true; }

void Player::ClearDirty() { dirty_ = false; }

}  // namespace server
