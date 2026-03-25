#ifndef CLIENT_APP_GAME_APP_H_
#define CLIENT_APP_GAME_APP_H_

#include <string>
#include <vector>

#include "client/controller/player_controller.h"
#include "client/controller/skill_controller.h"
#include "client/debug/dev_panel.h"
#include "client/model/model_root.h"
#include "client/net/network_manager.h"
#include "client/protocol/protocol_dispatcher.h"
#include "client/scene/scene_manager.h"
#include "client/ui/ui_manager.h"

namespace client {

class GameApp {
 public:
  GameApp();

  void RunFrame();

  NetworkManager& network_manager() { return network_manager_; }
  ProtocolDispatcher& protocol_dispatcher() { return protocol_dispatcher_; }
  ModelRoot& model_root() { return model_root_; }
  const ModelRoot& model_root() const { return model_root_; }
  SceneManager& scene_manager() { return scene_manager_; }
  const SceneManager& scene_manager() const { return scene_manager_; }
  UiManager& ui_manager() { return ui_manager_; }
  const UiManager& ui_manager() const { return ui_manager_; }
  DevPanel& dev_panel() { return dev_panel_; }
  const DevPanel& dev_panel() const { return dev_panel_; }

 private:
  void BindProtocolHandlers();
  void HandleEnterSceneSnapshot(
      const protocol::EnterSceneSnapshotMessage& message);
  void HandleSelfState(const protocol::SelfStateMessage& message);
  void HandleAoiEnter(const protocol::AoiEnterMessage& message);
  void HandleAoiLeave(const protocol::AoiLeaveMessage& message);
  void HandleInventoryDelta(const protocol::InventoryDeltaMessage& message);
  void PumpNetwork();
  void HandleInput();
  void UpdateModels();
  void UpdateScene();
  void UpdateUi();
  void Render();
  void UpdateDevPanel();
  void AppendProtocolSummary(std::string summary);

  NetworkManager network_manager_;
  ProtocolDispatcher protocol_dispatcher_;
  ModelRoot model_root_;
  SceneManager scene_manager_;
  UiManager ui_manager_{&model_root_};
  PlayerController player_controller_;
  SkillController skill_controller_;
  DevPanel dev_panel_;
  std::vector<std::string> recent_protocol_summaries_;
};

}  // namespace client

#endif  // CLIENT_APP_GAME_APP_H_
