#ifndef CLIENT_APP_GAME_APP_H_
#define CLIENT_APP_GAME_APP_H_

#include "client/net/network_manager.h"
#include "client/protocol/protocol_dispatcher.h"

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
};

}  // namespace client

#endif  // CLIENT_APP_GAME_APP_H_
