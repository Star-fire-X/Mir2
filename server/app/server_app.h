#ifndef SERVER_APP_SERVER_APP_H_
#define SERVER_APP_SERVER_APP_H_

#include <cstdint>
#include <optional>
#include <unordered_map>
#include <variant>
#include <vector>

#include "server/config/config_manager.h"
#include "server/net/session.h"
#include "server/player/player_manager.h"
#include "server/protocol/protocol_dispatcher.h"
#include "server/scene/drop_system.h"
#include "server/scene/movement_system.h"
#include "server/scene/scene_manager.h"
#include "server/skill/skill_system.h"
#include "server/battle/battle_system.h"
#include "shared/protocol/auth_messages.h"
#include "shared/protocol/scene_messages.h"
#include "shared/types/entity_id.h"

namespace server {

class ServerApp {
 public:
  struct SelfStateEvent {
    shared::EntityId entity_id{};
    shared::ScenePosition position{};
  };

  struct AoiEnterEvent {
    shared::VisibleEntitySnapshot entity{};
  };

  struct AoiLeaveEvent {
    shared::EntityId entity_id{};
  };

  using OutboundEvent = std::variant<shared::EnterSceneSnapshot, SelfStateEvent,
                                     AoiEnterEvent, AoiLeaveEvent,
                                     shared::InventoryDelta>;

  explicit ServerApp(const ConfigManager& config_manager);

  bool Init();
  shared::LoginResponse Login(Session* session,
                              const shared::LoginRequest& login_request);
  std::vector<OutboundEvent> EnterScene(
      Session* session, const shared::EnterSceneRequest& enter_scene_request);
  std::vector<OutboundEvent> HandleMove(
      const Session* session, const shared::MoveRequest& move_request,
      float delta_seconds);
  std::vector<OutboundEvent> HandleCastSkill(
      Session* session, const shared::CastSkillRequest& cast_skill_request,
      float now_seconds);
  std::vector<OutboundEvent> HandlePickup(
      const Session* session, const shared::PickupRequest& pickup_request);

 private:
  struct MonsterRuntimeTemplate {
    std::uint32_t drop_item_id = 0;
  };

  struct PlayerAndScene {
    Player* player = nullptr;
    Scene* scene = nullptr;
  };

  void BootstrapScene(std::uint32_t scene_id);
  void PrimeControlledEntityRuntime(Player* player);
  PlayerAndScene FindBoundPlayerAndScene(const Session* session);
  std::vector<OutboundEvent> FlushMoveEvents(
      const Player* player, Scene* scene,
      const std::optional<shared::ScenePosition>& previous_position);
  static std::vector<OutboundEvent> BuildEnterSceneEvents(
      const shared::EnterSceneSnapshot& snapshot);
  static shared::InventoryDelta BuildInventoryDelta(
      shared::PlayerId player_id, const Inventory& inventory);

  const ConfigManager& config_manager_;
  PlayerManager player_manager_;
  SceneManager scene_manager_;
  ProtocolDispatcher protocol_dispatcher_{&player_manager_, &scene_manager_};
  MovementSystem movement_system_;
  SkillSystem skill_system_;
  BattleSystem battle_system_;
  DropSystem drop_system_;
  std::unordered_map<std::uint32_t, MonsterRuntimeTemplate>
      monster_templates_by_id_;
  std::unordered_map<std::uint32_t, bool> bootstrapped_scenes_;
  std::uint64_t next_monster_entity_id_ = 900000;
};

}  // namespace server

#endif  // SERVER_APP_SERVER_APP_H_
