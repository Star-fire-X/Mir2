#ifndef SHARED_PROTOCOL_RUNTIME_MESSAGES_H_
#define SHARED_PROTOCOL_RUNTIME_MESSAGES_H_

#include <cstdint>
#include <string>

#include "shared/protocol/message_ids.h"
#include "shared/protocol/scene_messages.h"
#include "shared/types/entity_id.h"
#include "shared/types/player_id.h"

namespace shared {

struct SceneChannelBootstrap {
  static constexpr MessageId kMessageId = MessageId::kSceneChannelBootstrap;
  PlayerId player_id;
  std::uint32_t scene_id = 0;
  std::uint32_t kcp_conv = 0;
  std::uint16_t udp_port = 0;
  std::string session_token;
};

struct SelfState {
  static constexpr MessageId kMessageId = MessageId::kSelfState;
  EntityId entity_id;
  ScenePosition position;
};

struct AoiEnter {
  static constexpr MessageId kMessageId = MessageId::kAoiEnter;
  VisibleEntitySnapshot entity;
};

struct AoiLeave {
  static constexpr MessageId kMessageId = MessageId::kAoiLeave;
  EntityId entity_id;
};

}  // namespace shared

#endif  // SHARED_PROTOCOL_RUNTIME_MESSAGES_H_
