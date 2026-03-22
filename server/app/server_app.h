#ifndef SERVER_APP_SERVER_APP_H_
#define SERVER_APP_SERVER_APP_H_

#include <string>
#include <vector>

#include "server/config/config_manager.h"

namespace server {

class ServerApp {
 public:
  ServerApp() = default;

  bool Init(const GameConfig& config);
  bool IsInitialized() const;
  const std::vector<std::string>& ValidationErrors() const;

 private:
  ConfigManager config_manager_;
  bool is_initialized_ = false;
};

}  // namespace server

#endif  // SERVER_APP_SERVER_APP_H_
