#include "server/player/player.h"

#include <utility>

namespace server {

Player::Player(CharacterData data) : data_(std::move(data)) {}

const CharacterData& Player::data() const { return data_; }

CharacterData& Player::mutable_data() { return data_; }

Session* Player::session() const { return session_; }

bool Player::dirty() const { return dirty_; }

bool Player::operations_frozen() const { return operations_frozen_; }

const std::optional<shared::EntityId>& Player::controlled_entity_id() const {
  return controlled_entity_id_;
}

std::uint32_t Player::current_scene_id() const { return current_scene_id_; }

void Player::BindSession(Session* session) { session_ = session; }

void Player::SetControlledEntity(shared::EntityId entity_id,
                                 std::uint32_t scene_id) {
  controlled_entity_id_ = entity_id;
  current_scene_id_ = scene_id;
  operations_frozen_ = false;
}

void Player::ClearControlledEntity() {
  controlled_entity_id_.reset();
  current_scene_id_ = 0;
}

bool Player::CaptureSceneSnapshot(const Scene* scene) {
  if (scene == nullptr || !controlled_entity_id_.has_value()) {
    return false;
  }

  const std::optional<shared::ScenePosition> position =
      scene->GetPosition(*controlled_entity_id_);
  if (!position.has_value()) {
    return false;
  }

  data_.last_scene_snapshot.scene_id = current_scene_id_;
  data_.last_scene_snapshot.position = WorldPosition{position->x, position->y};
  return true;
}

void Player::SubmitMove(Scene* scene, const shared::MoveRequest& move_request) {
  if (scene == nullptr || operations_frozen_ || session_ == nullptr ||
      session_->state() != SessionState::kInScene) {
    return;
  }

  scene->Enqueue(SceneCommand{
      SceneCommandType::kMove,
      move_request,
  });
}

void Player::MarkDirty() { dirty_ = true; }

void Player::ClearDirty() { dirty_ = false; }

void Player::FreezeOperations() { operations_frozen_ = true; }

void Player::ResumeOperations() { operations_frozen_ = false; }

}  // namespace server
