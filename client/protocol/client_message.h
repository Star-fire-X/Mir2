#ifndef CLIENT_PROTOCOL_CLIENT_MESSAGE_H_
#define CLIENT_PROTOCOL_CLIENT_MESSAGE_H_

#include <variant>

#include "shared/protocol/scene_messages.h"

namespace client {
namespace protocol {

struct EnterSceneSnapshotMessage {
  shared::EnterSceneSnapshot snapshot;
};

struct SelfStateMessage {
  shared::EntityId entity_id;
  shared::ScenePosition position;
};

struct AoiEnterMessage {
  shared::VisibleEntitySnapshot entity;
};

struct AoiLeaveMessage {
  shared::EntityId entity_id;
};

struct InventoryDeltaMessage {
  shared::InventoryDelta delta;
};

using EnterScene = EnterSceneSnapshotMessage;
using SelfState = SelfStateMessage;
using AoiIn = AoiEnterMessage;
using AoiOut = AoiLeaveMessage;
using Delta = InventoryDeltaMessage;

using ClientMessage = std::variant<EnterScene, SelfState, AoiIn, AoiOut, Delta>;

}  // namespace protocol
}  // namespace client

#endif  // CLIENT_PROTOCOL_CLIENT_MESSAGE_H_
