#ifndef CLIENT_APP_GAME_APP_H_
#define CLIENT_APP_GAME_APP_H_

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
  GameApp() = default;

  void RunFrame();

  NetworkManager& network_manager() { return network_manager_; }
  ProtocolDispatcher& protocol_dispatcher() { return protocol_dispatcher_; }

 private:
  void PumpNetwork();
  void HandleInput();
  void UpdateModels();
  void UpdateScene();
  void UpdateUi();
  void Render();

  NetworkManager network_manager_;
  ProtocolDispatcher protocol_dispatcher_;
  ModelRoot model_root_;
  SceneManager scene_manager_;
  UiManager ui_manager_{&model_root_};
  PlayerController player_controller_;
  SkillController skill_controller_;
  DevPanel dev_panel_;
};

}  // namespace client

#endif  // CLIENT_APP_GAME_APP_H_
