#include "client/app/game_app.h"

#include <chrono>
#include <string>
#include <thread>  // NOLINT(build/c++11)
#include <utility>

namespace client {

GameApp::GameApp() { BindProtocolHandlers(); }

GameApp::~GameApp() { Stop(); }

void GameApp::Run() {
  running_ = true;
  network_manager_.Start();
  if (!login_requested_) {
    network_manager_.QueueOutbound(
        protocol::OutboundMessage{shared::LoginRequest{"hero", "pw"}});
    login_requested_ = true;
  }
  while (running_) {
    RunFrame();
    std::this_thread::sleep_for(std::chrono::milliseconds(16));
  }
}

void GameApp::Stop() {
  running_ = false;
  network_manager_.Stop();
}

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
  protocol_dispatcher_.SetLoginResponseHandler(
      [this](const protocol::LoginResponseMessage& message) {
        HandleLoginResponse(message);
      });
  protocol_dispatcher_.SetSceneChannelBootstrapHandler(
      [this](const protocol::SceneChannelBootstrapMessage& message) {
        HandleSceneChannelBootstrap(message);
      });
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
  protocol_dispatcher_.SetCastSkillResultHandler(
      [this](const protocol::CastSkillResultMessage& message) {
        HandleCastSkillResult(message);
      });
  protocol_dispatcher_.SetPickupResultHandler(
      [this](const protocol::PickupResultMessage& message) {
        HandlePickupResult(message);
      });
}

void GameApp::HandleLoginResponse(
    const protocol::LoginResponseMessage& message) {
  if (message.response.error_code == shared::ErrorCode::kOk) {
    player_id_ = message.response.player_id;
    if (!enter_scene_requested_) {
      network_manager_.QueueOutbound(protocol::OutboundMessage{
          shared::EnterSceneRequest{player_id_, 1}});
      enter_scene_requested_ = true;
    }
    AppendProtocolSummary("login_response");
    return;
  }
  AppendProtocolSummary("login_response_error");
}

void GameApp::HandleSceneChannelBootstrap(
    const protocol::SceneChannelBootstrapMessage& message) {
  if (message.bootstrap.scene_id != 0) {
    model_root_.scene_state_model().SetSceneId(message.bootstrap.scene_id);
  }
  AppendProtocolSummary("scene_channel_bootstrap");
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

  scene->EnterAoi(message.event.entity);
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

  scene->LeaveAoi(message.event.entity_id);
  model_root_.scene_state_model().SetVisibleEntityCount(scene->ViewCount());
  AppendProtocolSummary("aoi_leave");
}

void GameApp::HandleInventoryDelta(
    const protocol::InventoryDeltaMessage& message) {
  ui_manager_.HandleInventoryDelta(message);
  AppendProtocolSummary("inventory_delta");
}

void GameApp::HandleCastSkillResult(
    const protocol::CastSkillResultMessage& message) {
  if (message.result.error_code == shared::ErrorCode::kOk) {
    AppendProtocolSummary("cast_skill_result");
    return;
  }
  AppendProtocolSummary("cast_skill_result_error");
}

void GameApp::HandlePickupResult(const protocol::PickupResultMessage& message) {
  if (message.result.error_code == shared::ErrorCode::kOk) {
    AppendProtocolSummary("pickup_result");
    return;
  }
  AppendProtocolSummary("pickup_result_error");
}

void GameApp::PumpNetwork() {
  for (const protocol::ClientMessage& message : network_manager_.DrainInbound()) {
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
