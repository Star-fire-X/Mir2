#ifndef SERVER_PROTOCOL_PROTOCOL_DISPATCHER_H_
#define SERVER_PROTOCOL_PROTOCOL_DISPATCHER_H_

#include <cstdint>
#include <optional>

#include "server/aoi/aoi_system.h"
#include "server/player/player_manager.h"
#include "server/scene/scene_manager.h"
#include "shared/protocol/auth_messages.h"
#include "shared/protocol/scene_messages.h"

namespace server {

enum class SaveTrigger : std::uint8_t;
class SaveService;
class Session;

class ProtocolDispatcher {
 public:
  ProtocolDispatcher(PlayerManager* player_manager,
                     SceneManager* scene_manager);

  shared::LoginResponse HandleLogin(Session* session,
                                    const shared::LoginRequest& login_request);
  bool CanEnterScene(Session* session,
                     const shared::EnterSceneRequest& enter_scene_request) const;

  std::optional<shared::EnterSceneSnapshot> HandleEnterScene(
      Session* session, const shared::EnterSceneRequest& enter_scene_request);

  bool HandleMoveRequest(const Session* session,
                         const shared::MoveRequest& move_request);
  bool HandleLogout(Session* session, SaveService* save_service);
  bool HandleDisconnect(Session* session, SaveService* save_service);
  std::optional<shared::EnterSceneSnapshot> HandleReconnect(
      Session* session, shared::PlayerId player_id);

 private:
  Player* FindEnterScenePlayer(
      Session* session,
      const shared::EnterSceneRequest& enter_scene_request) const;
  CharacterData BuildDefaultCharacter(
      shared::PlayerId player_id,
      const shared::LoginRequest& login_request) const;
  bool TearDownSession(Session* session, SaveService* save_service,
                       SaveTrigger save_trigger);
  void DestroyControlledEntity(Player* player);

  PlayerManager* player_manager_ = nullptr;
  SceneManager* scene_manager_ = nullptr;
  std::uint64_t next_player_id_value_ = 10000;
};

}  // namespace server

#endif  // SERVER_PROTOCOL_PROTOCOL_DISPATCHER_H_
