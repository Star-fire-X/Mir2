#include "server/protocol/protocol_dispatcher.h"

#include <optional>

#include "server/db/save_service.h"
#include "server/entity/entity_factory.h"
#include "server/net/session.h"
#include "server/player/player.h"

namespace server {
namespace {

bool IsBoundToSession(const Player* player, const Session* session) {
  return player != nullptr && session != nullptr &&
         player->session() == session;
}

}  // namespace

ProtocolDispatcher::ProtocolDispatcher(PlayerManager* player_manager,
                                       SceneManager* scene_manager)
    : player_manager_(player_manager), scene_manager_(scene_manager) {}

shared::LoginResponse ProtocolDispatcher::HandleLogin(
    Session* session, const shared::LoginRequest& login_request) {
  if (session == nullptr) {
    return shared::LoginResponse{
        shared::ErrorCode::kInvalidCredentials,
        shared::PlayerId{},
    };
  }

  const shared::PlayerId player_id{next_player_id_value_++};
  Player& player = player_manager_->Upsert(
      player_id, BuildDefaultCharacter(player_id, login_request));
  player.BindSession(session);

  session->Authenticate(player_id);
  session->SelectCharacter();

  return shared::LoginResponse{
      shared::ErrorCode::kOk,
      player_id,
  };
}

bool ProtocolDispatcher::CanEnterScene(
    Session* session,
    const shared::EnterSceneRequest& enter_scene_request) const {
  return FindEnterScenePlayer(session, enter_scene_request) != nullptr;
}

std::optional<shared::EnterSceneSnapshot> ProtocolDispatcher::HandleEnterScene(
    Session* session, const shared::EnterSceneRequest& enter_scene_request) {
  Player* player = FindEnterScenePlayer(session, enter_scene_request);
  if (player == nullptr) {
    return std::nullopt;
  }

  DestroyControlledEntity(player);
  Scene& scene = scene_manager_->Emplace(enter_scene_request.scene_id);
  EntityFactory entity_factory(&scene);
  const shared::EntityId controlled_entity_id{
      static_cast<std::uint64_t>(enter_scene_request.player_id.value + 100000)};
  entity_factory.SpawnPlayer(player->data(), controlled_entity_id);
  player->SetControlledEntity(controlled_entity_id,
                              enter_scene_request.scene_id);
  player->ResumeOperations();
  session->EnterScene();

  AoiSystem aoi_system;
  return aoi_system.BuildEnterSceneSnapshot(
      enter_scene_request.player_id, controlled_entity_id,
      enter_scene_request.scene_id, scene);
}

bool ProtocolDispatcher::HandleMoveRequest(
    const Session* session, const shared::MoveRequest& move_request) {
  if (session == nullptr || session->state() != SessionState::kInScene ||
      !session->player_id().has_value()) {
    return false;
  }

  Player* player = player_manager_->Find(*session->player_id());
  if (!IsBoundToSession(player, session) ||
      !player->controlled_entity_id().has_value()) {
    return false;
  }
  if (player->operations_frozen()) {
    return false;
  }
  if (*player->controlled_entity_id() != move_request.entity_id) {
    return false;
  }

  Scene* scene = scene_manager_->Find(player->current_scene_id());
  if (scene == nullptr) {
    return false;
  }

  player->SubmitMove(scene, move_request);
  return true;
}

bool ProtocolDispatcher::HandleLogout(Session* session,
                                      SaveService* save_service) {
  return TearDownSession(session, save_service, SaveTrigger::kLogout);
}

bool ProtocolDispatcher::HandleDisconnect(Session* session,
                                          SaveService* save_service) {
  return TearDownSession(session, save_service, SaveTrigger::kDisconnect);
}

std::optional<shared::EnterSceneSnapshot> ProtocolDispatcher::HandleReconnect(
    Session* session, shared::PlayerId player_id) {
  if (session == nullptr) {
    return std::nullopt;
  }

  Player* player = player_manager_->Find(player_id);
  if (player == nullptr) {
    return std::nullopt;
  }

  if (session->player_id().has_value() && *session->player_id() != player_id) {
    return std::nullopt;
  }

  if (!session->player_id().has_value()) {
    session->Authenticate(player_id);
  }
  if (session->state() == SessionState::kAuthenticated) {
    session->SelectCharacter();
  }

  Session* previous_session = player->session();
  if (previous_session != nullptr && previous_session != session) {
    previous_session->Disconnect();
  }
  player->BindSession(session);
  player->ResumeOperations();
  return HandleEnterScene(
      session, shared::EnterSceneRequest{
                   player_id, player->data().last_scene_snapshot.scene_id});
}

Player* ProtocolDispatcher::FindEnterScenePlayer(
    Session* session,
    const shared::EnterSceneRequest& enter_scene_request) const {
  if (session == nullptr || !session->player_id().has_value()) {
    return nullptr;
  }
  if (*session->player_id() != enter_scene_request.player_id) {
    return nullptr;
  }

  Player* player = player_manager_->Find(enter_scene_request.player_id);
  if (!IsBoundToSession(player, session)) {
    return nullptr;
  }
  return player;
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

bool ProtocolDispatcher::TearDownSession(Session* session,
                                         SaveService* save_service,
                                         SaveTrigger save_trigger) {
  if (session == nullptr || !session->player_id().has_value()) {
    return false;
  }

  Player* player = player_manager_->Find(*session->player_id());
  if (!IsBoundToSession(player, session)) {
    return false;
  }

  const Scene* scene = scene_manager_->Find(player->current_scene_id());
  player->CaptureSceneSnapshot(scene);
  if (save_service != nullptr) {
    if (save_service->ShouldMarkDirty(save_trigger)) {
      player->MarkDirty();
    }
    if (save_service->ShouldQueueSnapshot(save_trigger, player->dirty())) {
      save_service->QueueSnapshotFromMainThread(player->data());
      player->ClearDirty();
    }
  }

  player->FreezeOperations();
  DestroyControlledEntity(player);
  player->BindSession(nullptr);
  session->Disconnect();
  return true;
}

void ProtocolDispatcher::DestroyControlledEntity(Player* player) {
  if (player == nullptr || !player->controlled_entity_id().has_value()) {
    if (player != nullptr) {
      player->ClearControlledEntity();
    }
    return;
  }

  Scene* scene = scene_manager_->Find(player->current_scene_id());
  if (scene != nullptr) {
    scene->DestroyEntity(*player->controlled_entity_id());
  }
  player->ClearControlledEntity();
}

}  // namespace server
