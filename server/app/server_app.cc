#include "server/app/server_app.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <optional>
#include <utility>
#include <vector>

#include "server/aoi/aoi_system.h"
#include "server/config/config_validator.h"
#include "server/ecs/components.h"
#include "server/entity/entity_factory.h"

namespace server {
namespace {

float DistanceBetween(const shared::ScenePosition& lhs,
                      const shared::ScenePosition& rhs) {
  const float dx = rhs.x - lhs.x;
  const float dy = rhs.y - lhs.y;
  return std::sqrt((dx * dx) + (dy * dy));
}

}  // namespace

ServerApp::ServerApp(const ConfigManager& config_manager)
    : config_manager_(config_manager), movement_system_(8.0F) {}

bool ServerApp::Init() {
  if (!config_manager_.IsLoaded()) {
    return false;
  }
  if (!ConfigValidator::Validate(config_manager_.GetConfig())) {
    return false;
  }

  monster_templates_by_id_.clear();
  for (const MonsterTemplate& monster_template :
       config_manager_.GetConfig().monster_templates) {
    monster_templates_by_id_[monster_template.id] =
        MonsterRuntimeTemplate{monster_template.drop_item_id};
  }

  for (const SkillTemplate& skill_template :
       config_manager_.GetConfig().skill_templates) {
    skill_system_.RegisterSkill(SkillDefinition{
        skill_template.id,
        static_cast<float>(skill_template.range),
        5U,
        0.5F,
        100,
    });
  }

  return true;
}

shared::LoginResponse ServerApp::Login(
    Session* session, const shared::LoginRequest& login_request) {
  return protocol_dispatcher_.HandleLogin(session, login_request);
}

std::vector<ServerApp::OutboundEvent> ServerApp::EnterScene(
    Session* session, const shared::EnterSceneRequest& enter_scene_request) {
  if (!protocol_dispatcher_.CanEnterScene(session, enter_scene_request)) {
    return {};
  }

  BootstrapScene(enter_scene_request.scene_id);

  const std::optional<shared::EnterSceneSnapshot> snapshot =
      protocol_dispatcher_.HandleEnterScene(session, enter_scene_request);
  if (!snapshot.has_value()) {
    return {};
  }

  Player* player = player_manager_.Find(enter_scene_request.player_id);
  PrimeControlledEntityRuntime(player);
  return BuildEnterSceneEvents(*snapshot);
}

std::vector<ServerApp::OutboundEvent> ServerApp::HandleMove(
    const Session* session, const shared::MoveRequest& move_request,
    float delta_seconds) {
  const PlayerAndScene player_and_scene = FindBoundPlayerAndScene(session);
  if (player_and_scene.player == nullptr || player_and_scene.scene == nullptr) {
    return {};
  }

  if (!protocol_dispatcher_.HandleMoveRequest(session, move_request)) {
    return {};
  }

  const std::optional<shared::ScenePosition> previous_position =
      player_and_scene.player->controlled_entity_id().has_value()
          ? player_and_scene.scene->GetPosition(
                *player_and_scene.player->controlled_entity_id())
          : std::nullopt;
  player_and_scene.scene->Tick(&movement_system_, delta_seconds);
  return FlushMoveEvents(player_and_scene.player, player_and_scene.scene,
                         previous_position);
}

std::vector<ServerApp::OutboundEvent> ServerApp::HandleCastSkill(
    Session* session, const shared::CastSkillRequest& cast_skill_request,
    float now_seconds) {
  const PlayerAndScene player_and_scene = FindBoundPlayerAndScene(session);
  if (player_and_scene.player == nullptr || player_and_scene.scene == nullptr ||
      !player_and_scene.player->controlled_entity_id().has_value()) {
    return {};
  }

  Scene& scene = *player_and_scene.scene;
  const shared::EntityId caster_entity_id =
      *player_and_scene.player->controlled_entity_id();
  if (caster_entity_id != cast_skill_request.caster_entity_id) {
    return {};
  }

  const std::optional<entt::entity> caster_entity =
      scene.Find(caster_entity_id);
  const std::optional<entt::entity> target_entity =
      scene.Find(cast_skill_request.target_entity_id);
  if (!caster_entity.has_value() || !target_entity.has_value()) {
    return {};
  }

  auto& registry = scene.registry();
  if (!registry.all_of<ecs::SkillRuntimeComponent>(*caster_entity) ||
      !registry.all_of<ecs::CombatStateComponent>(*target_entity) ||
      !registry.all_of<ecs::MonsterRefComponent>(*target_entity)) {
    return {};
  }

  const std::optional<shared::ScenePosition> caster_position =
      scene.GetPosition(caster_entity_id);
  const std::optional<shared::ScenePosition> target_position =
      scene.GetPosition(cast_skill_request.target_entity_id);
  if (!caster_position.has_value() || !target_position.has_value()) {
    return {};
  }

  ecs::SkillRuntimeComponent& skill_runtime =
      registry.get<ecs::SkillRuntimeComponent>(*caster_entity);
  SkillRuntimeState runtime_state;
  runtime_state.current_mp = skill_runtime.current_mp;
  runtime_state.cooldown_until_by_skill_id =
      skill_runtime.cooldown_until_by_skill_id;

  const shared::ErrorCode validation = skill_system_.ValidateAndConsume(
      cast_skill_request, DistanceBetween(*caster_position, *target_position),
      now_seconds, &runtime_state);
  if (validation != shared::ErrorCode::kOk) {
    return {};
  }

  skill_runtime.current_mp = runtime_state.current_mp;
  skill_runtime.cooldown_until_by_skill_id =
      std::move(runtime_state.cooldown_until_by_skill_id);

  ecs::CombatStateComponent& combat_state =
      registry.get<ecs::CombatStateComponent>(*target_entity);
  const std::optional<SkillDefinition> skill_definition =
      skill_system_.FindSkill(cast_skill_request.skill_id);
  if (!skill_definition.has_value()) {
    return {};
  }

  const shared::CastSkillResult result = battle_system_.ResolveSkillHit(
      cast_skill_request, skill_definition->base_damage, &combat_state);
  if (result.error_code != shared::ErrorCode::kOk || !combat_state.is_dead) {
    return {};
  }

  const ecs::MonsterRefComponent monster_ref =
      registry.get<ecs::MonsterRefComponent>(*target_entity);
  const auto monster_template =
      monster_templates_by_id_.find(monster_ref.monster_template_id);
  if (monster_template == monster_templates_by_id_.end()) {
    return {};
  }

  const std::vector<shared::EntityId> drop_entity_ids =
      drop_system_.SpawnDropsForDefeatedMonster(
          &scene, cast_skill_request.target_entity_id,
          {DropItemSpec{monster_template->second.drop_item_id, 1U}});

  std::vector<OutboundEvent> events;
  events.push_back(AoiLeaveEvent{cast_skill_request.target_entity_id});
  for (shared::EntityId drop_entity_id : drop_entity_ids) {
    const std::optional<shared::VisibleEntitySnapshot> drop_snapshot =
        scene.BuildVisibleSnapshot(drop_entity_id);
    if (drop_snapshot.has_value()) {
      events.push_back(AoiEnterEvent{*drop_snapshot});
    }
  }
  return events;
}

std::vector<ServerApp::OutboundEvent> ServerApp::HandlePickup(
    const Session* session, const shared::PickupRequest& pickup_request) {
  const PlayerAndScene player_and_scene = FindBoundPlayerAndScene(session);
  if (player_and_scene.player == nullptr || player_and_scene.scene == nullptr) {
    return {};
  }

  const shared::PickupResult result = drop_system_.HandlePickup(
      player_and_scene.scene, player_and_scene.player, pickup_request, 20U);
  if (result.error_code != shared::ErrorCode::kOk) {
    return {};
  }

  player_and_scene.player->MarkDirty();
  return {
      AoiLeaveEvent{pickup_request.drop_entity_id},
      BuildInventoryDelta(pickup_request.player_id,
                          player_and_scene.player->data().inventory),
  };
}

void ServerApp::BootstrapScene(std::uint32_t scene_id) {
  if (bootstrapped_scenes_[scene_id]) {
    return;
  }

  Scene& scene = scene_manager_.Emplace(scene_id);
  EntityFactory entity_factory(&scene);
  std::size_t spawn_index = 0;
  for (const MonsterSpawn& monster_spawn :
       config_manager_.GetConfig().monster_spawns) {
    entity_factory.SpawnMonster(
        monster_spawn.monster_template_id,
        shared::EntityId{next_monster_entity_id_++},
        shared::ScenePosition{7.0F + (static_cast<float>(spawn_index) * 4.0F),
                              4.0F});
    ++spawn_index;
  }
  bootstrapped_scenes_[scene_id] = true;
}

void ServerApp::PrimeControlledEntityRuntime(Player* player) {
  if (player == nullptr || !player->controlled_entity_id().has_value()) {
    return;
  }

  Scene* scene = scene_manager_.Find(player->current_scene_id());
  if (scene == nullptr) {
    return;
  }

  const std::optional<entt::entity> controlled_entity =
      scene->Find(*player->controlled_entity_id());
  if (!controlled_entity.has_value()) {
    return;
  }

  auto& skill_runtime =
      scene->registry().get<ecs::SkillRuntimeComponent>(*controlled_entity);
  if (skill_runtime.current_mp == 0) {
    skill_runtime.current_mp = 100U;
  }
}

ServerApp::PlayerAndScene ServerApp::FindBoundPlayerAndScene(
    const Session* session) {
  if (session == nullptr || !session->player_id().has_value()) {
    return {};
  }

  Player* player = player_manager_.Find(*session->player_id());
  if (player == nullptr || player->session() != session) {
    return {};
  }

  Scene* scene = scene_manager_.Find(player->current_scene_id());
  return PlayerAndScene{player, scene};
}

std::vector<ServerApp::OutboundEvent> ServerApp::FlushMoveEvents(
    const Player* player, Scene* scene,
    const std::optional<shared::ScenePosition>& previous_position) {
  if (player == nullptr || scene == nullptr ||
      !player->controlled_entity_id().has_value()) {
    return {};
  }

  std::vector<OutboundEvent> events;
  const auto& corrections = scene->recent_move_corrections();
  if (!corrections.empty()) {
    std::transform(
        corrections.begin(), corrections.end(), std::back_inserter(events),
        [](const shared::MoveCorrection& correction) {
          return OutboundEvent{SelfStateEvent{
              correction.entity_id, correction.authoritative_position}};
        });
    scene->ClearRecentMoveCorrections();
    return events;
  }

  const std::optional<shared::ScenePosition> position =
      scene->GetPosition(*player->controlled_entity_id());
  if (position.has_value()) {
    events.push_back(
        SelfStateEvent{*player->controlled_entity_id(), *position});
  }

  if (previous_position.has_value() && position.has_value()) {
    AoiSystem aoi_system;
    const AoiDiff diff = aoi_system.ComputeEnterLeave(
        *player->controlled_entity_id(), *previous_position, *position, *scene);
    std::transform(diff.left.begin(), diff.left.end(),
                   std::back_inserter(events), [](shared::EntityId entity_id) {
                     return OutboundEvent{AoiLeaveEvent{entity_id}};
                   });
    std::transform(diff.entered.begin(), diff.entered.end(),
                   std::back_inserter(events),
                   [](const shared::VisibleEntitySnapshot& entity) {
                     return OutboundEvent{AoiEnterEvent{entity}};
                   });
  }
  return events;
}

std::vector<ServerApp::OutboundEvent> ServerApp::BuildEnterSceneEvents(
    const shared::EnterSceneSnapshot& snapshot) {
  return {snapshot};
}

shared::InventoryDelta ServerApp::BuildInventoryDelta(
    shared::PlayerId player_id, const Inventory& inventory) {
  shared::InventoryDelta delta;
  delta.player_id = player_id;
  for (std::size_t index = 0; index < inventory.slots().size(); ++index) {
    const auto& slot = inventory.slots()[index];
    if (!slot.has_value()) {
      continue;
    }

    delta.slots.push_back(shared::InventorySlotDelta{
        static_cast<std::uint32_t>(index),
        slot->item_template_id,
        slot->item_count,
    });
  }
  return delta;
}

}  // namespace server
