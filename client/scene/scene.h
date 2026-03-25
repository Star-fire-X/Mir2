#ifndef CLIENT_SCENE_SCENE_H_
#define CLIENT_SCENE_SCENE_H_

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

#include "client/protocol/client_message.h"
#include "client/view/drop_view.h"
#include "client/view/entity_view.h"
#include "client/view/monster_view.h"
#include "client/view/player_view.h"
#include "shared/protocol/scene_messages.h"
#include "shared/types/entity_id.h"

namespace client {

class Scene {
 public:
  void EnterAoi(const shared::VisibleEntitySnapshot& entity) {
    views_[entity.entity_id] = CreateView(entity);
  }

  void LeaveAoi(shared::EntityId entity_id) { views_.erase(entity_id); }

  void ApplySelfState(const protocol::SelfStateMessage& self_state) {
    EntityView* view = FindView(self_state.state.entity_id);
    if (view != nullptr) {
      view->ApplyDelta(self_state.state.position);
    }
  }

  [[nodiscard]] EntityView* FindView(shared::EntityId entity_id) {
    const auto it = views_.find(entity_id);
    if (it == views_.end()) {
      return nullptr;
    }
    return it->second.get();
  }

  [[nodiscard]] const EntityView* FindView(shared::EntityId entity_id) const {
    const auto it = views_.find(entity_id);
    if (it == views_.end()) {
      return nullptr;
    }
    return it->second.get();
  }

  [[nodiscard]] std::size_t ViewCount() const { return views_.size(); }

  [[nodiscard]] std::vector<EntityView*> ViewList() {
    std::vector<EntityView*> view_list;
    view_list.reserve(views_.size());
    for (auto& [_, view] : views_) {
      view_list.push_back(view.get());
    }
    return view_list;
  }

  [[nodiscard]] std::vector<const EntityView*> ViewList() const {
    std::vector<const EntityView*> view_list;
    view_list.reserve(views_.size());
    for (const auto& [_, view] : views_) {
      view_list.push_back(view.get());
    }
    return view_list;
  }

 private:
  struct EntityIdHash {
    std::size_t operator()(const shared::EntityId& entity_id) const noexcept {
      return std::hash<std::uint64_t>{}(entity_id.value);
    }
  };

  static std::unique_ptr<EntityView> CreateView(
      const shared::VisibleEntitySnapshot& entity) {
    switch (entity.kind) {
      case shared::VisibleEntityKind::kPlayer:
        return std::make_unique<PlayerView>(entity);
      case shared::VisibleEntityKind::kMonster:
        return std::make_unique<MonsterView>(entity);
      case shared::VisibleEntityKind::kDrop:
        return std::make_unique<DropView>(entity);
      case shared::VisibleEntityKind::kUnknown:
      default:
        return std::make_unique<EntityView>(entity.entity_id, entity.position,
                                            EntityViewKind::kUnknown);
    }
  }

  std::unordered_map<shared::EntityId, std::unique_ptr<EntityView>,
                     EntityIdHash>
      views_;
};

}  // namespace client

#endif  // CLIENT_SCENE_SCENE_H_
