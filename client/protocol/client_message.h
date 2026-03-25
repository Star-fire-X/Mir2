#ifndef CLIENT_PROTOCOL_CLIENT_MESSAGE_H_
#define CLIENT_PROTOCOL_CLIENT_MESSAGE_H_

#include <variant>

#include "shared/protocol/auth_messages.h"
#include "shared/protocol/combat_messages.h"
#include "shared/protocol/runtime_messages.h"
#include "shared/protocol/scene_messages.h"

namespace client {
namespace protocol {

struct LoginResponseMessage {
  shared::LoginResponse response;
};

struct SceneChannelBootstrapMessage {
  shared::SceneChannelBootstrap bootstrap;
};

struct EnterSceneSnapshotMessage {
  shared::EnterSceneSnapshot snapshot;
};

struct SelfStateMessage {
  shared::SelfState state;
};

struct AoiEnterMessage {
  shared::AoiEnter event;
};

struct AoiLeaveMessage {
  shared::AoiLeave event;
};

struct InventoryDeltaMessage {
  shared::InventoryDelta delta;
};

struct CastSkillResultMessage {
  shared::CastSkillResult result;
};

struct PickupResultMessage {
  shared::PickupResult result;
};

using InboundMessage =
    std::variant<LoginResponseMessage, SceneChannelBootstrapMessage,
                 EnterSceneSnapshotMessage, SelfStateMessage, AoiEnterMessage,
                 AoiLeaveMessage, InventoryDeltaMessage, CastSkillResultMessage,
                 PickupResultMessage>;

using OutboundMessage =
    std::variant<shared::LoginRequest, shared::EnterSceneRequest,
                 shared::MoveRequest, shared::CastSkillRequest,
                 shared::PickupRequest>;

using ClientMessage = InboundMessage;

}  // namespace protocol
}  // namespace client

#endif  // CLIENT_PROTOCOL_CLIENT_MESSAGE_H_
