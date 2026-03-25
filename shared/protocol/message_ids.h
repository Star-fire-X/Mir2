#ifndef SHARED_PROTOCOL_MESSAGE_IDS_H_
#define SHARED_PROTOCOL_MESSAGE_IDS_H_

#include <cstdint>

namespace shared {

enum class MessageId : std::uint16_t {
  kLoginRequest = 1000,
  kLoginResponse = 1001,
  kSceneChannelBootstrap = 1002,
  kEnterSceneRequest = 2000,
  kEnterSceneSnapshot = 2001,
  kMoveRequest = 2002,
  kMoveCorrection = 2003,
  kPickupRequest = 2004,
  kPickupResult = 2005,
  kInventoryDelta = 2006,
  kSelfState = 2007,
  kAoiEnter = 2008,
  kAoiLeave = 2009,
  kCastSkillRequest = 3000,
  kCastSkillResult = 3001,
};

}  // namespace shared

#endif  // SHARED_PROTOCOL_MESSAGE_IDS_H_
