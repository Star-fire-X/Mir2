#include <chrono>  // NOLINT(build/c++11)
#include <cstdlib>
#include <thread>  // NOLINT(build/c++11)

#include "SDL.h"
#include "client/app/game_app.h"
#include "gtest/gtest.h"
#include "server/config/config_manager.h"
#include "server/runtime/server_runtime.h"

#if defined(__has_feature)
#if __has_feature(address_sanitizer)
#define CLIENT_TEST_HAS_LSAN 1
#endif
#endif

#if defined(__SANITIZE_ADDRESS__) && !defined(CLIENT_TEST_HAS_LSAN)
#define CLIENT_TEST_HAS_LSAN 1
#endif

#if !defined(CLIENT_TEST_HAS_LSAN)
#define CLIENT_TEST_HAS_LSAN 0
#endif

#if CLIENT_TEST_HAS_LSAN
#include <sanitizer/lsan_interface.h>
#endif

namespace client {
namespace {

class ScopedLsanDisabler {
 public:
  ScopedLsanDisabler() {
#if CLIENT_TEST_HAS_LSAN
    __lsan_disable();
#endif
  }

  ~ScopedLsanDisabler() {
#if CLIENT_TEST_HAS_LSAN
    __lsan_enable();
#endif
  }
};

server::GameConfig MakeValidConfig() {
  server::GameConfig config;
  config.item_templates.push_back(server::ItemTemplate{.id = 5001});
  config.skill_templates.push_back(
      server::SkillTemplate{.id = 1001, .range = 3});
  config.monster_templates.push_back(server::MonsterTemplate{
      .id = 2001,
      .drop_item_id = 5001,
      .move_speed = 1,
      .skill_id = 1001,
  });
  config.monster_spawns.push_back(
      server::MonsterSpawn{.monster_template_id = 2001});
  return config;
}

TEST(GameAppSdlTest, DummyDriverClickMovesPlayerThroughNetworkFlow) {
  ASSERT_EQ(setenv("SDL_VIDEODRIVER", "dummy", 1), 0);

  server::ConfigManager config_manager;
  config_manager.Load(MakeValidConfig());
  server::ServerRuntime runtime(config_manager, server::ServerRuntime::Options{
                                                    .tcp_port = 0,
                                                    .udp_port = 0,
                                                });
  ASSERT_TRUE(runtime.Start());

  GameApp app(NetworkConfig{
      .host = "127.0.0.1",
      .tcp_port = runtime.tcp_port(),
  });
  std::thread run_thread([&app]() {
    ScopedLsanDisabler lsan_disabler;
    app.Run();
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(250));
  SDL_Event click;
  click.type = SDL_MOUSEBUTTONDOWN;
  click.button.button = SDL_BUTTON_LEFT;
  click.button.x = 320;
  click.button.y = 224;
  ASSERT_EQ(SDL_PushEvent(&click), 1);

  std::this_thread::sleep_for(std::chrono::milliseconds(250));
  app.Stop();
  run_thread.join();

  EXPECT_FLOAT_EQ(app.model_root().player_model().position().x, 6.0F);
  EXPECT_FLOAT_EQ(app.model_root().player_model().position().y, 4.0F);
  runtime.Stop();
}

}  // namespace
}  // namespace client
