#ifndef SERVER_APP_SERVER_APP_H_
#define SERVER_APP_SERVER_APP_H_

#include "server/config/config_manager.h"

namespace server {

class ServerApp {
 public:
  ServerApp();
  explicit ServerApp(ConfigManager config_manager);

  bool Init();

 private:
  static ConfigManager BuildDefaultConfigManager();

  ConfigManager config_manager_;
};

}  // namespace server

#endif  // SERVER_APP_SERVER_APP_H_
