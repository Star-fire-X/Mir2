#include "server/app/server_app.h"

int main() {
  server::ServerApp app;
  return app.Init() ? 0 : 1;
}
