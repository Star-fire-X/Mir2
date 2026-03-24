#ifndef CLIENT_MODEL_INVENTORY_MODEL_H_
#define CLIENT_MODEL_INVENTORY_MODEL_H_

#include <cstddef>
#include <cstdint>
#include <functional>
#include <utility>
#include <vector>

#include "shared/protocol/scene_messages.h"

namespace client {

struct InventorySlotState {
  std::uint32_t item_template_id = 0;
  std::uint32_t item_count = 0;
  bool occupied = false;
};

class InventoryModel {
 public:
  using Observer = std::function<void(const InventoryModel&)>;

  explicit InventoryModel(std::size_t slot_count = 20) : slots_(slot_count) {}

  void SetObserver(Observer observer) { observer_ = std::move(observer); }

  void ApplyDelta(const shared::InventoryDelta& delta) {
    bool changed = false;
    for (const shared::InventorySlotDelta& slot_delta : delta.slots) {
      const std::size_t slot_index =
          static_cast<std::size_t>(slot_delta.slot_index);
      if (slot_index >= slots_.size()) {
        continue;
      }

      InventorySlotState& slot = slots_[slot_index];
      slot.item_template_id = slot_delta.item_template_id;
      slot.item_count = slot_delta.item_count;
      slot.occupied =
          slot_delta.item_template_id != 0 && slot_delta.item_count != 0;
      changed = true;
    }

    if (changed && observer_) {
      observer_(*this);
    }
  }

  [[nodiscard]] const std::vector<InventorySlotState>& slots() const {
    return slots_;
  }

 private:
  std::vector<InventorySlotState> slots_;
  Observer observer_;
};

}  // namespace client

#endif  // CLIENT_MODEL_INVENTORY_MODEL_H_
