#include "client/app/game_app.h"

#include <string>
#include <utility>

namespace client {

GameApp::GameApp() { BindProtocolHandlers(); }

void GameApp::RunFrame() {
  PumpNetwork();
  HandleInput();
  UpdateModels();
  UpdateScene();
  UpdateUi();
  Render();
  UpdateDevPanel();
}

void GameApp::BindProtocolHandlers() {
  protocol_dispatcher_.SetEnterSceneSnapshotHandler(
      [this](const protocol::EnterSceneSnapshotMessage& message) {
        HandleEnterSceneSnapshot(message);
      });
  protocol_dispatcher_.SetSelfStateHandler(
      [this](const protocol::SelfStateMessage& message) {
        HandleSelfState(message);
      });
  protocol_dispatcher_.SetAoiEnterHandler(
      [this](const protocol::AoiEnterMessage& message) {
        HandleAoiEnter(message);
      });
  protocol_dispatcher_.SetAoiLeaveHandler(
      [this](const protocol::AoiLeaveMessage& message) {
        HandleAoiLeave(message);
      });
  protocol_dispatcher_.SetInventoryDeltaHandler(
      [this](const protocol::InventoryDeltaMessage& message) {
        HandleInventoryDelta(message);
      });
}

void GameApp::HandleEnterSceneSnapshot(
    const protocol::EnterSceneSnapshotMessage& message) {
  const shared::EnterSceneSnapshot& snapshot = message.snapshot;
  const std::uint32_t previous_scene_id =
      model_root_.scene_state_model().scene_id();
  if (previous_scene_id != 0 && previous_scene_id != snapshot.scene_id) {
    scene_manager_.Remove(previous_scene_id);
  }

  scene_manager_.Remove(snapshot.scene_id);
  Scene& scene = scene_manager_.Emplace(snapshot.scene_id);
  model_root_.player_model().ApplyEnterSceneSnapshot(snapshot);
  model_root_.scene_state_model().SetSceneId(snapshot.scene_id);

  for (const shared::VisibleEntitySnapshot& entity :
       snapshot.visible_entities) {
    scene.EnterAoi(entity);
  }

  model_root_.scene_state_model().SetVisibleEntityCount(scene.ViewCount());
  AppendProtocolSummary("enter_scene_snapshot");
}

void GameApp::HandleSelfState(const protocol::SelfStateMessage& message) {
  model_root_.player_model().ApplySelfState(message);

  Scene* scene =
      scene_manager_.Find(model_root_.scene_state_model().scene_id());
  if (scene != nullptr) {
    scene->ApplySelfState(message);
    model_root_.scene_state_model().SetVisibleEntityCount(scene->ViewCount());
  }

  AppendProtocolSummary("self_state");
}

void GameApp::HandleAoiEnter(const protocol::AoiEnterMessage& message) {
  Scene* scene =
      scene_manager_.Find(model_root_.scene_state_model().scene_id());
  if (scene == nullptr) {
    AppendProtocolSummary("aoi_enter");
    return;
  }

  scene->EnterAoi(message.entity);
  model_root_.scene_state_model().SetVisibleEntityCount(scene->ViewCount());
  AppendProtocolSummary("aoi_enter");
}

void GameApp::HandleAoiLeave(const protocol::AoiLeaveMessage& message) {
  Scene* scene =
      scene_manager_.Find(model_root_.scene_state_model().scene_id());
  if (scene == nullptr) {
    AppendProtocolSummary("aoi_leave");
    return;
  }

  scene->LeaveAoi(message.entity_id);
  model_root_.scene_state_model().SetVisibleEntityCount(scene->ViewCount());
  AppendProtocolSummary("aoi_leave");
}

void GameApp::HandleInventoryDelta(
    const protocol::InventoryDeltaMessage& message) {
  ui_manager_.HandleInventoryDelta(message);
  AppendProtocolSummary("inventory_delta");
}

void GameApp::PumpNetwork() {
  for (const protocol::ClientMessage& message : network_manager_.Drain()) {
    protocol_dispatcher_.Dispatch(message);
  }
}

void GameApp::HandleInput() {}

void GameApp::UpdateModels() {}

void GameApp::UpdateScene() {}

void GameApp::UpdateUi() {}

void GameApp::Render() {}

void GameApp::UpdateDevPanel() {
  DevPanelSnapshot snapshot;
  snapshot.scene_id = model_root_.scene_state_model().scene_id();
  snapshot.entity_count =
      model_root_.scene_state_model().visible_entity_count();
  snapshot.player_position = model_root_.player_model().position();
  snapshot.recent_protocol_summaries = recent_protocol_summaries_;
  dev_panel_.Update(std::move(snapshot));
}

void GameApp::AppendProtocolSummary(std::string summary) {
  recent_protocol_summaries_.push_back(std::move(summary));
  constexpr std::size_t kMaxProtocolSummaries = 8;
  if (recent_protocol_summaries_.size() > kMaxProtocolSummaries) {
    recent_protocol_summaries_.erase(recent_protocol_summaries_.begin());
  }
}

}  // namespace client
