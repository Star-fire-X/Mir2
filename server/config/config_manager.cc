#include "server/config/config_manager.h"

namespace server {

bool ConfigManager::Load(const GameConfig& config) {
  validation_errors_ = ConfigValidator::Validate(config);
  if (!validation_errors_.empty()) {
    is_loaded_ = false;
    return false;
  }

  config_ = config;
  is_loaded_ = true;
  return true;
}

bool ConfigManager::IsLoaded() const { return is_loaded_; }

const GameConfig& ConfigManager::GetConfig() const { return config_; }

const std::vector<std::string>& ConfigManager::ValidationErrors() const {
  return validation_errors_;
}

}  // namespace server
