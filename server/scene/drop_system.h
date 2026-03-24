#ifndef SERVER_SCENE_DROP_SYSTEM_H_
#define SERVER_SCENE_DROP_SYSTEM_H_

#include <compare>
#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "server/player/player.h"
#include "server/scene/scene.h"
#include "shared/protocol/scene_messages.h"

namespace server {

struct DropItemSpec {
  std::uint32_t item_template_id = 0;
  std::uint32_t item_count = 0;
};

class DropSystem {
 public:
  explicit DropSystem(std::uint64_t first_drop_entity_id = 500000);

  [[nodiscard]] std::vector<shared::EntityId> SpawnDrops(
      Scene* scene, const shared::ScenePosition& drop_position,
      const std::vector<DropItemSpec>& drop_items);

  [[nodiscard]] std::vector<shared::EntityId> SpawnDropsForDefeatedMonster(
      Scene* scene, shared::EntityId defeated_monster_entity_id,
      const std::vector<DropItemSpec>& drop_items);

  [[nodiscard]] shared::PickupResult HandlePickup(
      Scene* scene, Player* player, const shared::PickupRequest& pickup_request,
      std::uint32_t max_stack_size);

 private:
  struct DropEntry {
    std::uint32_t item_template_id = 0;
    std::uint32_t item_count = 0;
  };

  struct EntityIdHash {
    std::size_t operator()(const shared::EntityId& entity_id) const noexcept {
      return std::hash<std::uint64_t>{}(entity_id.value);
    }
  };

  struct PickupCacheKey {
    std::uint64_t player_id = 0;
    std::uint64_t drop_entity_id = 0;
    std::uint32_t client_seq = 0;
    auto operator<=>(const PickupCacheKey&) const = default;
  };

  struct PickupCacheKeyHash {
    std::size_t operator()(const PickupCacheKey& key) const noexcept {
      std::size_t hash = std::hash<std::uint64_t>{}(key.player_id);
      hash ^= std::hash<std::uint64_t>{}(key.drop_entity_id) + 0x9e3779b9U +
              (hash << 6U) + (hash >> 2U);
      hash ^= std::hash<std::uint32_t>{}(key.client_seq) + 0x9e3779b9U +
              (hash << 6U) + (hash >> 2U);
      return hash;
    }
  };

  std::uint64_t next_drop_entity_id_ = 500000;
  std::unordered_map<shared::EntityId, DropEntry, EntityIdHash> drops_;
  std::unordered_set<PickupCacheKey, PickupCacheKeyHash> pickup_cache_;
};

}  // namespace server

#endif  // SERVER_SCENE_DROP_SYSTEM_H_
