#ifndef SERVER_APP_SERVER_APP_H_
#define SERVER_APP_SERVER_APP_H_

#include "server/config/config_manager.h"

namespace server {

class ServerApp {
 public:
  explicit ServerApp(const ConfigManager& config_manager);

  bool Init();

 private:
  const ConfigManager& config_manager_;
};

}  // namespace server

#endif  // SERVER_APP_SERVER_APP_H_
