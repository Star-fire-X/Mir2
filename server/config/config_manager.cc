#include "server/config/config_manager.h"

namespace server {

bool ConfigManager::IsLoaded() const {
  return is_loaded_;
}

}  // namespace server
