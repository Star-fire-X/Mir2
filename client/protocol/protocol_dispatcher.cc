#include "client/protocol/protocol_dispatcher.h"

#include <type_traits>
#include <utility>

namespace client {

void ProtocolDispatcher::SetEnterSceneSnapshotHandler(
    EnterSceneSnapshotHandler handler) {
  enter_scene_snapshot_handler_ = std::move(handler);
}

void ProtocolDispatcher::SetSelfStateHandler(SelfStateHandler handler) {
  self_state_handler_ = std::move(handler);
}

void ProtocolDispatcher::SetAoiEnterHandler(AoiEnterHandler handler) {
  aoi_enter_handler_ = std::move(handler);
}

void ProtocolDispatcher::SetAoiLeaveHandler(AoiLeaveHandler handler) {
  aoi_leave_handler_ = std::move(handler);
}

void ProtocolDispatcher::SetInventoryDeltaHandler(
    InventoryDeltaHandler handler) {
  inventory_delta_handler_ = std::move(handler);
}

void ProtocolDispatcher::Dispatch(
    const protocol::ClientMessage& message) const {
  std::visit(
      [this](const auto& payload) {
        using Payload = std::decay_t<decltype(payload)>;
        if constexpr (std::is_same_v<Payload,
                                     protocol::EnterSceneSnapshotMessage>) {
          if (enter_scene_snapshot_handler_) {
            enter_scene_snapshot_handler_(payload);
          }
        } else if constexpr (std::is_same_v<Payload, protocol::SelfStateMessage>) {
          if (self_state_handler_) {
            self_state_handler_(payload);
          }
        } else if constexpr (std::is_same_v<Payload, protocol::AoiEnterMessage>) {
          if (aoi_enter_handler_) {
            aoi_enter_handler_(payload);
          }
        } else if constexpr (std::is_same_v<Payload, protocol::AoiLeaveMessage>) {
          if (aoi_leave_handler_) {
            aoi_leave_handler_(payload);
          }
        } else if constexpr (std::is_same_v<Payload,
                                             protocol::InventoryDeltaMessage>) {
          if (inventory_delta_handler_) {
            inventory_delta_handler_(payload);
          }
        }
      },
      message);
}

}  // namespace client
