#ifndef SERVER_PLAYER_PLAYER_MANAGER_H_
#define SERVER_PLAYER_PLAYER_MANAGER_H_

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <unordered_map>
#include <utility>

#include "server/data/character_data.h"
#include "server/player/player.h"
#include "shared/types/player_id.h"

namespace server {

class PlayerManager {
 public:
  Player& Upsert(shared::PlayerId player_id, CharacterData data) {
    const auto existing = players_.find(player_id);
    if (existing != players_.end()) {
      existing->second->mutable_data() = std::move(data);
      return *(existing->second);
    }

    auto [inserted, _] =
        players_.emplace(player_id, std::make_unique<Player>(std::move(data)));
    return *(inserted->second);
  }

  Player* Find(shared::PlayerId player_id) {
    const auto it = players_.find(player_id);
    if (it == players_.end()) {
      return nullptr;
    }
    return it->second.get();
  }

  const Player* Find(shared::PlayerId player_id) const {
    const auto it = players_.find(player_id);
    if (it == players_.end()) {
      return nullptr;
    }
    return it->second.get();
  }

  bool Remove(shared::PlayerId player_id) {
    return players_.erase(player_id) > 0;
  }

  [[nodiscard]] std::size_t size() const { return players_.size(); }

 private:
  struct PlayerIdHash {
    std::size_t operator()(const shared::PlayerId& player_id) const noexcept {
      return std::hash<std::uint64_t>{}(player_id.value);
    }
  };

  std::unordered_map<shared::PlayerId, std::unique_ptr<Player>, PlayerIdHash>
      players_;
};

}  // namespace server

#endif  // SERVER_PLAYER_PLAYER_MANAGER_H_
