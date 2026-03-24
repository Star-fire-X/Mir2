#ifndef CLIENT_SCENE_SCENE_MANAGER_H_
#define CLIENT_SCENE_SCENE_MANAGER_H_

#include <cstdint>
#include <memory>
#include <unordered_map>

#include "client/scene/scene.h"

namespace client {

class SceneManager {
 public:
  Scene& Emplace(std::uint32_t scene_id) {
    auto [it, _] = scenes_.try_emplace(scene_id, std::make_unique<Scene>());
    return *(it->second);
  }

  Scene* Find(std::uint32_t scene_id) {
    const auto it = scenes_.find(scene_id);
    if (it == scenes_.end()) {
      return nullptr;
    }
    return it->second.get();
  }

  const Scene* Find(std::uint32_t scene_id) const {
    const auto it = scenes_.find(scene_id);
    if (it == scenes_.end()) {
      return nullptr;
    }
    return it->second.get();
  }

  bool Remove(std::uint32_t scene_id) { return scenes_.erase(scene_id) > 0; }

 private:
  std::unordered_map<std::uint32_t, std::unique_ptr<Scene>> scenes_;
};

}  // namespace client

#endif  // CLIENT_SCENE_SCENE_MANAGER_H_
