#include "server/app/server_app.h"
#include "server/config/config_manager.h"

int main() {
  server::ConfigManager config_manager;
  server::ServerApp app(config_manager);
  return app.Init() ? 0 : 1;
}
