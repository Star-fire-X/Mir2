#include "server/protocol/protocol_dispatcher.h"

#include <optional>

#include "server/entity/entity_factory.h"
#include "server/net/session.h"
#include "server/player/player.h"

namespace server {

ProtocolDispatcher::ProtocolDispatcher(PlayerManager* player_manager,
                                       SceneManager* scene_manager)
    : player_manager_(player_manager), scene_manager_(scene_manager) {}

shared::LoginResponse ProtocolDispatcher::HandleLogin(
    Session& session, const shared::LoginRequest& login_request) {
  const shared::PlayerId player_id{next_player_id_value_++};
  Player& player = player_manager_->Upsert(
      player_id, BuildDefaultCharacter(player_id, login_request));
  player.BindSession(&session);

  session.Authenticate(player_id);
  session.SelectCharacter();

  return shared::LoginResponse{
      shared::ErrorCode::kOk,
      player_id,
  };
}

std::optional<shared::EnterSceneSnapshot> ProtocolDispatcher::HandleEnterScene(
    Session& session, const shared::EnterSceneRequest& enter_scene_request) {
  if (!session.player_id().has_value()) {
    return std::nullopt;
  }
  if (*session.player_id() != enter_scene_request.player_id) {
    return std::nullopt;
  }

  Player* player = player_manager_->Find(enter_scene_request.player_id);
  if (player == nullptr) {
    return std::nullopt;
  }

  Scene& scene = scene_manager_->Emplace(enter_scene_request.scene_id);
  EntityFactory entity_factory(&scene);
  const shared::EntityId controlled_entity_id{
      static_cast<std::uint64_t>(enter_scene_request.player_id.value + 100000)};
  entity_factory.SpawnPlayer(player->data(), controlled_entity_id);
  player->SetControlledEntity(controlled_entity_id,
                              enter_scene_request.scene_id);
  session.EnterScene();

  AoiSystem aoi_system;
  return aoi_system.BuildEnterSceneSnapshot(
      enter_scene_request.player_id, controlled_entity_id,
      enter_scene_request.scene_id, scene);
}

bool ProtocolDispatcher::HandleMoveRequest(
    Session& session, const shared::MoveRequest& move_request) {
  if (session.state() != SessionState::kInScene ||
      !session.player_id().has_value()) {
    return false;
  }

  Player* player = player_manager_->Find(*session.player_id());
  if (player == nullptr || !player->controlled_entity_id().has_value()) {
    return false;
  }
  if (*player->controlled_entity_id() != move_request.entity_id) {
    return false;
  }

  Scene* scene = scene_manager_->Find(player->current_scene_id());
  if (scene == nullptr) {
    return false;
  }

  player->SubmitMove(*scene, move_request);
  return true;
}

CharacterData ProtocolDispatcher::BuildDefaultCharacter(
    shared::PlayerId player_id,
    const shared::LoginRequest& login_request) const {
  CharacterData data;
  data.identity.player_id = player_id;
  data.identity.character_name = login_request.account_name;
  data.base_attributes.level = 1;
  data.base_attributes.strength = 5;
  data.base_attributes.agility = 5;
  data.base_attributes.vitality = 5;
  data.base_attributes.intelligence = 5;
  data.last_scene_snapshot.scene_id = 1;
  data.last_scene_snapshot.position = {0.0F, 0.0F};
  data.version = 1;
  return data;
}

}  // namespace server
