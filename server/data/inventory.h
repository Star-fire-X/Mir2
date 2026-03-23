#ifndef SERVER_DATA_INVENTORY_H_
#define SERVER_DATA_INVENTORY_H_

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace server {

struct InventorySlot {
  std::uint32_t item_template_id{};
  std::uint32_t item_count{};
};

class Inventory {
 public:
  explicit Inventory(std::size_t slot_count = 20);

  bool AddStackableItem(std::uint32_t item_template_id,
                        std::uint32_t item_count, std::uint32_t max_stack_size);
  bool RemoveItemFromSlot(std::size_t slot_index);

  const std::vector<std::optional<InventorySlot>>& slots() const;

 private:
  std::vector<std::optional<InventorySlot>> slots_;
};

}  // namespace server

#endif  // SERVER_DATA_INVENTORY_H_
