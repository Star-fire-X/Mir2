#ifndef CLIENT_UI_UI_MANAGER_H_
#define CLIENT_UI_UI_MANAGER_H_

#include "client/model/model_root.h"
#include "client/protocol/client_message.h"
#include "client/ui/inventory_panel.h"

namespace client {

class UiManager {
 public:
  UiManager() = default;

  explicit UiManager(ModelRoot* model_root) { BindModelRoot(model_root); }

  void BindModelRoot(ModelRoot* model_root) {
    model_root_ = model_root;
    if (model_root_ == nullptr) {
      inventory_panel_.BindInventoryModel(nullptr);
      return;
    }
    inventory_panel_.BindInventoryModel(&model_root_->inventory_model());
  }

  void HandleInventoryDelta(
      const protocol::InventoryDeltaMessage& delta_message) {
    if (model_root_ == nullptr) {
      return;
    }
    model_root_->inventory_model().ApplyDelta(delta_message.delta);
  }

  [[nodiscard]] InventoryPanel& inventory_panel() { return inventory_panel_; }
  [[nodiscard]] const InventoryPanel& inventory_panel() const {
    return inventory_panel_;
  }

 private:
  ModelRoot* model_root_ = nullptr;
  InventoryPanel inventory_panel_;
};

}  // namespace client

#endif  // CLIENT_UI_UI_MANAGER_H_
