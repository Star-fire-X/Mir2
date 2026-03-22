#include "client/app/game_app.h"

namespace client {

void GameApp::RunFrame() {
  PumpNetwork();
  HandleInput();
  UpdateModels();
  UpdateScene();
  UpdateUi();
  Render();
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

}  // namespace client
