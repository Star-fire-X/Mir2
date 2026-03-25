#include "client/platform/sdl_runtime.h"

namespace client {

SdlRuntime::~SdlRuntime() { Shutdown(); }

bool SdlRuntime::Initialize(const Options& options) {
  Shutdown();
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
    return false;
  }

  width_ = options.width;
  height_ = options.height;
  const Uint32 window_flags =
      options.hidden ? static_cast<Uint32>(SDL_WINDOW_HIDDEN) : 0U;
  window_ = SDL_CreateWindow(options.title.c_str(), SDL_WINDOWPOS_CENTERED,
                             SDL_WINDOWPOS_CENTERED, options.width,
                             options.height, window_flags);
  if (window_ == nullptr) {
    Shutdown();
    return false;
  }

  renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_SOFTWARE);
  if (renderer_ == nullptr) {
    Shutdown();
    return false;
  }
  return true;
}

void SdlRuntime::Shutdown() {
  if (renderer_ != nullptr) {
    SDL_DestroyRenderer(renderer_);
    renderer_ = nullptr;
  }
  if (window_ != nullptr) {
    SDL_DestroyWindow(window_);
    window_ = nullptr;
  }
  if (SDL_WasInit(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0U) {
    SDL_Quit();
  }
  width_ = 0;
  height_ = 0;
}

bool SdlRuntime::ready() const {
  return window_ != nullptr && renderer_ != nullptr;
}

std::vector<SdlEvent> SdlRuntime::PollInput() {
  std::vector<SdlEvent> events;
  SDL_Event event;
  while (SDL_PollEvent(&event) == 1) {
    if (event.type == SDL_QUIT) {
      events.push_back(SdlEvent{SdlEventType::kQuit});
    } else if (event.type == SDL_MOUSEBUTTONDOWN &&
               event.button.button == SDL_BUTTON_LEFT) {
      events.push_back(
          SdlEvent{SdlEventType::kLeftClick, event.button.x, event.button.y});
    }
  }
  return events;
}

bool SdlRuntime::BeginFrame() {
  if (!ready()) {
    return false;
  }
  SDL_SetRenderDrawColor(renderer_, 16, 18, 22, 255);
  SDL_RenderClear(renderer_);
  return true;
}

void SdlRuntime::EndFrame() {
  if (!ready()) {
    return;
  }
  SDL_RenderPresent(renderer_);
}

}  // namespace client
