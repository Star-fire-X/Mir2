#ifndef SERVER_CONFIG_CONFIG_MANAGER_H_
#define SERVER_CONFIG_CONFIG_MANAGER_H_

#include <vector>

#include "server/config/config_validator.h"

namespace server {

class ConfigManager {
 public:
  ConfigManager() = default;

  bool Load(const GameConfig& config);
  bool IsLoaded() const;
  const GameConfig& GetConfig() const;
  const std::vector<std::string>& ValidationErrors() const;

 private:
  GameConfig config_;
  std::vector<std::string> validation_errors_;
  bool is_loaded_ = false;
};

}  // namespace server

#endif  // SERVER_CONFIG_CONFIG_MANAGER_H_
