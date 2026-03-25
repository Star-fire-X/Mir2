#include "client/protocol/protocol_dispatcher.h"

#include <type_traits>
#include <utility>

namespace client {
namespace {

template <typename>
inline constexpr bool kUnhandledClientMessage = false;

}  // namespace

void ProtocolDispatcher::SetLoginResponseHandler(LoginResponseHandler handler) {
  login_response_handler_ = std::move(handler);
}

void ProtocolDispatcher::SetSceneChannelBootstrapHandler(
    SceneChannelBootstrapHandler handler) {
  scene_channel_bootstrap_handler_ = std::move(handler);
}

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

void ProtocolDispatcher::SetCastSkillResultHandler(
    CastSkillResultHandler handler) {
  cast_skill_result_handler_ = std::move(handler);
}

void ProtocolDispatcher::SetPickupResultHandler(PickupResultHandler handler) {
  pickup_result_handler_ = std::move(handler);
}

void ProtocolDispatcher::Dispatch(
    const protocol::ClientMessage& message) const {
  std::visit(
      [this](const auto& payload) {
        using Payload = std::decay_t<decltype(payload)>;
        if constexpr (std::is_same_v<Payload, protocol::LoginResponseMessage>) {
          if (login_response_handler_) {
            login_response_handler_(payload);
          }
          return;
        }
        if constexpr (std::is_same_v<Payload,
                                     protocol::SceneChannelBootstrapMessage>) {
          if (scene_channel_bootstrap_handler_) {
            scene_channel_bootstrap_handler_(payload);
          }
          return;
        }
        if constexpr (std::is_same_v<Payload,
                                     protocol::EnterSceneSnapshotMessage>) {
          if (enter_scene_snapshot_handler_) {
            enter_scene_snapshot_handler_(payload);
          }
          return;
        }
        if constexpr (std::is_same_v<Payload, protocol::SelfStateMessage>) {
          if (self_state_handler_) {
            self_state_handler_(payload);
          }
          return;
        }
        if constexpr (std::is_same_v<Payload, protocol::AoiEnterMessage>) {
          if (aoi_enter_handler_) {
            aoi_enter_handler_(payload);
          }
          return;
        }
        if constexpr (std::is_same_v<Payload, protocol::AoiLeaveMessage>) {
          if (aoi_leave_handler_) {
            aoi_leave_handler_(payload);
          }
          return;
        }
        if constexpr (std::is_same_v<Payload,
                                     protocol::InventoryDeltaMessage>) {
          if (inventory_delta_handler_) {
            inventory_delta_handler_(payload);
          }
          return;
        }
        if constexpr (std::is_same_v<Payload,
                                     protocol::CastSkillResultMessage>) {
          if (cast_skill_result_handler_) {
            cast_skill_result_handler_(payload);
          }
          return;
        }
        if constexpr (std::is_same_v<Payload, protocol::PickupResultMessage>) {
          if (pickup_result_handler_) {
            pickup_result_handler_(payload);
          }
          return;
        }
        if constexpr (
            !std::is_same_v<Payload, protocol::LoginResponseMessage> &&
            !std::is_same_v<Payload, protocol::SceneChannelBootstrapMessage> &&
            !std::is_same_v<Payload, protocol::EnterSceneSnapshotMessage> &&
            !std::is_same_v<Payload, protocol::SelfStateMessage> &&
            !std::is_same_v<Payload, protocol::AoiEnterMessage> &&
            !std::is_same_v<Payload, protocol::AoiLeaveMessage> &&
            !std::is_same_v<Payload, protocol::InventoryDeltaMessage> &&
            !std::is_same_v<Payload, protocol::CastSkillResultMessage> &&
            !std::is_same_v<Payload, protocol::PickupResultMessage>) {
          static_assert(kUnhandledClientMessage<Payload>,
                        "ProtocolDispatcher::Dispatch is missing a "
                        "ClientMessage handler");
        }
      },
      message);
}

}  // namespace client
