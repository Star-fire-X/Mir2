#include "client/app/game_app.h"

#include <chrono>
#include <cstdlib>
#include <string>
#include <thread>  // NOLINT(build/c++11)
#include <utility>

#include "SDL.h"

namespace client {
namespace {

constexpr int kWorldOriginX = 32;
constexpr int kWorldOriginY = 32;
constexpr int kTileSizePx = 48;
constexpr int kEntityHalfSizePx = 10;
constexpr SDL_Rect kSkillButtonRect{640, 520, 120, 44};
constexpr SDL_Rect kPickupButtonRect{780, 520, 120, 44};

shared::ScenePosition ScreenToWorld(int x, int y) {
  return shared::ScenePosition{
      static_cast<float>(x - kWorldOriginX) / static_cast<float>(kTileSizePx),
      static_cast<float>(y - kWorldOriginY) / static_cast<float>(kTileSizePx),
  };
}

SDL_FRect WorldToScreen(const shared::ScenePosition& position) {
  return SDL_FRect{
      static_cast<float>(kWorldOriginX) + (position.x * kTileSizePx) -
          static_cast<float>(kEntityHalfSizePx),
      static_cast<float>(kWorldOriginY) + (position.y * kTileSizePx) -
          static_cast<float>(kEntityHalfSizePx),
      static_cast<float>(kEntityHalfSizePx * 2),
      static_cast<float>(kEntityHalfSizePx * 2),
  };
}

bool PointInRect(int x, int y, const SDL_Rect& rect) {
  return x >= rect.x && x < rect.x + rect.w && y >= rect.y &&
         y < rect.y + rect.h;
}

}  // namespace

GameApp::GameApp(NetworkConfig config) : network_manager_(std::move(config)) {
  player_controller_.SetMoveRequestSink(
      [this](const shared::MoveRequest& request) {
        network_manager_.QueueOutbound(protocol::OutboundMessage{request});
      });
  skill_controller_.SetCastSkillRequestSink(
      [this](const shared::CastSkillRequest& request) {
        network_manager_.QueueOutbound(protocol::OutboundMessage{request});
      });
  BindProtocolHandlers();
}

GameApp::~GameApp() { Stop(); }

void GameApp::Run() {
  running_ = true;
  sdl_runtime_.Initialize(SdlRuntime::Options{
      .title = "mir2",
      .width = 960,
      .height = 640,
      .hidden = false,
  });
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
  sdl_runtime_.Shutdown();
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

void GameApp::HandleInput() {
  if (!sdl_runtime_.ready()) {
    return;
  }

  for (const SdlEvent& event : sdl_runtime_.PollInput()) {
    if (event.type == SdlEventType::kQuit) {
      running_ = false;
      continue;
    }
    if (event.type != SdlEventType::kLeftClick) {
      continue;
    }

    if (PointInRect(event.x, event.y, kSkillButtonRect)) {
      if (selected_entity_id_.value != 0 &&
          model_root_.player_model().controlled_entity_id().value != 0) {
        skill_controller_.HandleSkillButton(
            model_root_.player_model().controlled_entity_id(),
            selected_entity_id_, 1001, next_client_seq_++);
      }
      continue;
    }

    if (PointInRect(event.x, event.y, kPickupButtonRect)) {
      if (selected_entity_id_.value != 0 && player_id_.value != 0) {
        network_manager_.QueueOutbound(protocol::OutboundMessage{
            shared::PickupRequest{player_id_, selected_entity_id_,
                                  next_client_seq_++}});
      }
      continue;
    }

    Scene* scene =
        scene_manager_.Find(model_root_.scene_state_model().scene_id());
    if (scene != nullptr) {
      for (const EntityView* view : scene->ViewList()) {
        const SDL_FRect rect = WorldToScreen(view->render_position());
        const float click_x = static_cast<float>(event.x);
        const float click_y = static_cast<float>(event.y);
        if (click_x >= rect.x && click_x <= rect.x + rect.w &&
            click_y >= rect.y && click_y <= rect.y + rect.h) {
          selected_entity_id_ = view->entity_id();
          return;
        }
      }
    }

    if (model_root_.player_model().controlled_entity_id().value != 0) {
      player_controller_.HandleMoveInput(
          model_root_.player_model().controlled_entity_id(),
          ScreenToWorld(event.x, event.y), next_client_seq_++, SDL_GetTicks64());
    }
  }
}

void GameApp::UpdateModels() {}

void GameApp::UpdateScene() {
  Scene* scene =
      scene_manager_.Find(model_root_.scene_state_model().scene_id());
  if (scene == nullptr) {
    return;
  }

  for (const EntityView* view : scene->ViewList()) {
    const_cast<EntityView*>(view)->UpdateInterpolation(1.0F);
  }
}

void GameApp::UpdateUi() {}

void GameApp::Render() {
  if (!sdl_runtime_.BeginFrame()) {
    return;
  }

  SDL_Renderer* renderer = sdl_runtime_.renderer();
  if (renderer == nullptr) {
    return;
  }

  SDL_SetRenderDrawColor(renderer, 48, 52, 60, 255);
  for (int x = 0; x <= 12; ++x) {
    SDL_RenderDrawLine(renderer, kWorldOriginX + (x * kTileSizePx),
                       kWorldOriginY,
                       kWorldOriginX + (x * kTileSizePx),
                       kWorldOriginY + (8 * kTileSizePx));
  }
  for (int y = 0; y <= 8; ++y) {
    SDL_RenderDrawLine(renderer, kWorldOriginX,
                       kWorldOriginY + (y * kTileSizePx),
                       kWorldOriginX + (12 * kTileSizePx),
                       kWorldOriginY + (y * kTileSizePx));
  }

  Scene* scene =
      scene_manager_.Find(model_root_.scene_state_model().scene_id());
  if (scene != nullptr) {
    for (const EntityView* view : scene->ViewList()) {
      const SDL_FRect rect = WorldToScreen(view->render_position());
      switch (view->kind()) {
        case EntityViewKind::kPlayer:
          SDL_SetRenderDrawColor(renderer, 63, 185, 80, 255);
          break;
        case EntityViewKind::kMonster:
          SDL_SetRenderDrawColor(renderer, 214, 79, 68, 255);
          break;
        case EntityViewKind::kDrop:
          SDL_SetRenderDrawColor(renderer, 223, 188, 72, 255);
          break;
        case EntityViewKind::kUnknown:
        default:
          SDL_SetRenderDrawColor(renderer, 180, 180, 180, 255);
          break;
      }
      SDL_RenderFillRectF(renderer, &rect);
      if (view->entity_id() == selected_entity_id_) {
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDrawRectF(renderer, &rect);
      }
    }
  }

  SDL_SetRenderDrawColor(renderer, 34, 36, 42, 255);
  SDL_RenderFillRect(renderer, &kSkillButtonRect);
  SDL_RenderFillRect(renderer, &kPickupButtonRect);

  sdl_runtime_.EndFrame();
}

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
