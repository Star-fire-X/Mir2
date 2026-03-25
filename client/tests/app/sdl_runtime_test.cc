#include <cstdlib>
#include <vector>

#include "client/platform/sdl_runtime.h"
#include "gtest/gtest.h"

namespace client {
namespace {

TEST(SdlRuntimeTest, DummyDriverInitializesWindowAndRenderer) {
  ASSERT_EQ(setenv("SDL_VIDEODRIVER", "dummy", 1), 0);

  SdlRuntime runtime;
  ASSERT_TRUE(runtime.Initialize(SdlRuntime::Options{
      .title = "mir2-test",
      .width = 640,
      .height = 480,
      .hidden = true,
  }));
  EXPECT_TRUE(runtime.ready());
  EXPECT_TRUE(runtime.BeginFrame());
  runtime.EndFrame();
  runtime.Shutdown();
  EXPECT_FALSE(runtime.ready());
}

TEST(SdlRuntimeTest, PollInputReturnsMouseClickEvent) {
  ASSERT_EQ(setenv("SDL_VIDEODRIVER", "dummy", 1), 0);

  SdlRuntime runtime;
  ASSERT_TRUE(runtime.Initialize(SdlRuntime::Options{
      .title = "mir2-test",
      .width = 640,
      .height = 480,
      .hidden = true,
  }));

  SDL_Event event;
  event.type = SDL_MOUSEBUTTONDOWN;
  event.button.button = SDL_BUTTON_LEFT;
  event.button.x = 120;
  event.button.y = 160;
  ASSERT_EQ(SDL_PushEvent(&event), 1);

  const std::vector<SdlEvent> events = runtime.PollInput();
  ASSERT_EQ(events.size(), 1U);
  EXPECT_EQ(events[0].type, SdlEventType::kLeftClick);
  EXPECT_EQ(events[0].x, 120);
  EXPECT_EQ(events[0].y, 160);

  runtime.Shutdown();
}

}  // namespace
}  // namespace client
