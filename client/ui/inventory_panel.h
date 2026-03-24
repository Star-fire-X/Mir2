#ifndef CLIENT_UI_INVENTORY_PANEL_H_
#define CLIENT_UI_INVENTORY_PANEL_H_

#include <cstddef>
#include <vector>

#include "client/model/inventory_model.h"

namespace client {

class InventoryPanel {
 public:
  ~InventoryPanel() {
    if (inventory_model_ != nullptr) {
      inventory_model_->SetObserver({});
    }
  }

  void BindInventoryModel(InventoryModel* inventory_model) {
    if (inventory_model_ != nullptr) {
      inventory_model_->SetObserver({});
    }

    inventory_model_ = inventory_model;
    if (inventory_model_ == nullptr) {
      rendered_slots_.clear();
      refresh_count_ = 0;
      return;
    }

    inventory_model_->SetObserver(
        [this](const InventoryModel&) { RefreshFromModel(); });
  }

  [[nodiscard]] std::size_t refresh_count() const { return refresh_count_; }

  [[nodiscard]] const std::vector<InventorySlotState>& rendered_slots() const {
    return rendered_slots_;
  }

 private:
  void RefreshFromModel() {
    if (inventory_model_ == nullptr) {
      return;
    }
    rendered_slots_ = inventory_model_->slots();
    ++refresh_count_;
  }

  InventoryModel* inventory_model_ = nullptr;
  std::vector<InventorySlotState> rendered_slots_;
  std::size_t refresh_count_ = 0;
};

}  // namespace client

#endif  // CLIENT_UI_INVENTORY_PANEL_H_
