#ifndef CLIENT_PROTOCOL_PROTOCOL_DISPATCHER_H_
#define CLIENT_PROTOCOL_PROTOCOL_DISPATCHER_H_

#include <functional>

#include "client/protocol/client_message.h"

namespace client {

class ProtocolDispatcher {
 public:
  using LoginResponseHandler =
      std::function<void(const protocol::LoginResponseMessage&)>;
  using SceneChannelBootstrapHandler =
      std::function<void(const protocol::SceneChannelBootstrapMessage&)>;
  using EnterSceneSnapshotHandler =
      std::function<void(const protocol::EnterSceneSnapshotMessage&)>;
  using SelfStateHandler =
      std::function<void(const protocol::SelfStateMessage&)>;
  using AoiEnterHandler = std::function<void(const protocol::AoiEnterMessage&)>;
  using AoiLeaveHandler = std::function<void(const protocol::AoiLeaveMessage&)>;
  using InventoryDeltaHandler =
      std::function<void(const protocol::InventoryDeltaMessage&)>;
  using CastSkillResultHandler =
      std::function<void(const protocol::CastSkillResultMessage&)>;
  using PickupResultHandler =
      std::function<void(const protocol::PickupResultMessage&)>;

  void SetLoginResponseHandler(LoginResponseHandler handler);
  void SetSceneChannelBootstrapHandler(SceneChannelBootstrapHandler handler);
  void SetEnterSceneSnapshotHandler(EnterSceneSnapshotHandler handler);
  void SetSelfStateHandler(SelfStateHandler handler);
  void SetAoiEnterHandler(AoiEnterHandler handler);
  void SetAoiLeaveHandler(AoiLeaveHandler handler);
  void SetInventoryDeltaHandler(InventoryDeltaHandler handler);
  void SetCastSkillResultHandler(CastSkillResultHandler handler);
  void SetPickupResultHandler(PickupResultHandler handler);

  void Dispatch(const protocol::ClientMessage& message) const;

 private:
  LoginResponseHandler login_response_handler_;
  SceneChannelBootstrapHandler scene_channel_bootstrap_handler_;
  EnterSceneSnapshotHandler enter_scene_snapshot_handler_;
  SelfStateHandler self_state_handler_;
  AoiEnterHandler aoi_enter_handler_;
  AoiLeaveHandler aoi_leave_handler_;
  InventoryDeltaHandler inventory_delta_handler_;
  CastSkillResultHandler cast_skill_result_handler_;
  PickupResultHandler pickup_result_handler_;
};

}  // namespace client

#endif  // CLIENT_PROTOCOL_PROTOCOL_DISPATCHER_H_
