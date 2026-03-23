#include "server/app/server_app.h"

#include "server/config/config_validator.h"

namespace server {

ServerApp::ServerApp(const ConfigManager& config_manager)
    : config_manager_(config_manager) {}

bool ServerApp::Init() {
  if (!config_manager_.IsLoaded()) {
    return false;
  }
  return ConfigValidator::Validate(config_manager_.GetConfig());
}

}  // namespace server
