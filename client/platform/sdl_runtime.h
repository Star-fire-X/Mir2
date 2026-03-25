#ifndef CLIENT_PLATFORM_SDL_RUNTIME_H_
#define CLIENT_PLATFORM_SDL_RUNTIME_H_

#include <cstdint>
#include <string>
#include <vector>

#include "SDL.h"

namespace client {

enum class SdlEventType : std::uint8_t {
  kUnknown = 0,
  kQuit = 1,
  kLeftClick = 2,
};

struct SdlEvent {
  SdlEventType type = SdlEventType::kUnknown;
  int x = 0;
  int y = 0;
};

class SdlRuntime {
 public:
  struct Options {
    std::string title = "mir2";
    int width = 960;
    int height = 640;
    bool hidden = false;
  };

  SdlRuntime() = default;
  ~SdlRuntime();

  bool Initialize(const Options& options);
  void Shutdown();
  [[nodiscard]] bool ready() const;
  [[nodiscard]] std::vector<SdlEvent> PollInput();
  bool BeginFrame();
  void EndFrame();

  [[nodiscard]] SDL_Renderer* renderer() const { return renderer_; }
  [[nodiscard]] int width() const { return width_; }
  [[nodiscard]] int height() const { return height_; }

 private:
  SDL_Window* window_ = nullptr;
  SDL_Renderer* renderer_ = nullptr;
  int width_ = 0;
  int height_ = 0;
};

}  // namespace client

#endif  // CLIENT_PLATFORM_SDL_RUNTIME_H_
