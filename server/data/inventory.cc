#include "server/data/inventory.h"

#include <algorithm>
#include <utility>

namespace server {

Inventory::Inventory(std::size_t slot_count) : slots_(slot_count) {}

bool Inventory::AddStackableItem(std::uint32_t item_template_id,
                                 std::uint32_t item_count,
                                 std::uint32_t max_stack_size) {
  if (item_template_id == 0 || item_count == 0 || max_stack_size == 0) {
    return false;
  }

  auto updated_slots = slots_;
  std::uint32_t remaining = item_count;

  for (auto& slot : updated_slots) {
    if (!slot.has_value() || slot->item_template_id != item_template_id ||
        slot->item_count >= max_stack_size) {
      continue;
    }

    const std::uint32_t space = max_stack_size - slot->item_count;
    const std::uint32_t added = std::min(space, remaining);
    slot->item_count += added;
    remaining -= added;
    if (remaining == 0) {
      slots_ = std::move(updated_slots);
      return true;
    }
  }

  for (auto& slot : updated_slots) {
    if (slot.has_value()) {
      continue;
    }

    const std::uint32_t added = std::min(max_stack_size, remaining);
    slot = InventorySlot{.item_template_id = item_template_id,
                         .item_count = added};
    remaining -= added;
    if (remaining == 0) {
      slots_ = std::move(updated_slots);
      return true;
    }
  }

  return false;
}

bool Inventory::RemoveItemFromSlot(std::size_t slot_index) {
  if (slot_index >= slots_.size() || !slots_[slot_index].has_value()) {
    return false;
  }

  slots_[slot_index].reset();
  return true;
}

const std::vector<std::optional<InventorySlot>>& Inventory::slots() const {
  return slots_;
}

}  // namespace server
