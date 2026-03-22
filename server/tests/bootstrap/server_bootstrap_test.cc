#include "gtest/gtest.h"
#include "server/config/config_manager.h"

namespace server {
namespace {

TEST(ServerBootstrapTest, ConfigManagerStartsEmpty) {
  const ConfigManager config_manager;
  EXPECT_FALSE(config_manager.IsLoaded());
}

}  // namespace
}  // namespace server
