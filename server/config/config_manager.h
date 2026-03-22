#ifndef SERVER_CONFIG_CONFIG_MANAGER_H_
#define SERVER_CONFIG_CONFIG_MANAGER_H_

#include "server/config/config_model.h"

namespace server {

class ConfigManager {
 public:
  ConfigManager() = default;
  explicit ConfigManager(GameConfig config);

  bool IsLoaded() const;
  const GameConfig& GetConfig() const;
  void Load(GameConfig config);

 private:
  bool is_loaded_ = false;
  GameConfig config_{};
};

}  // namespace server

#endif  // SERVER_CONFIG_CONFIG_MANAGER_H_
