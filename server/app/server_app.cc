#include "server/app/server_app.h"

#include <string>

namespace server {

bool ServerApp::Init(const GameConfig& config) {
  if (!config_manager_.Load(config)) {
    is_initialized_ = false;
    return false;
  }

  is_initialized_ = true;
  return true;
}

bool ServerApp::IsInitialized() const { return is_initialized_; }

const std::vector<std::string>& ServerApp::ValidationErrors() const {
  return config_manager_.ValidationErrors();
}

}  // namespace server
