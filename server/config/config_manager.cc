#include "server/config/config_manager.h"

#include <utility>

namespace server {

ConfigManager::ConfigManager(GameConfig config) { Load(std::move(config)); }

bool ConfigManager::IsLoaded() const { return is_loaded_; }

const GameConfig& ConfigManager::GetConfig() const { return config_; }

void ConfigManager::Load(GameConfig config) {
  config_ = std::move(config);
  is_loaded_ = true;
}

}  // namespace server
