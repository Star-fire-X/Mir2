#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>

#include "shared/protocol/message_ids.h"

namespace {

const char* MessageName(shared::MessageId message_id) {
  switch (message_id) {
    case shared::MessageId::kLoginRequest:
      return "LoginRequest";
    case shared::MessageId::kLoginResponse:
      return "LoginResponse";
    case shared::MessageId::kEnterSceneRequest:
      return "EnterSceneRequest";
    case shared::MessageId::kEnterSceneSnapshot:
      return "EnterSceneSnapshot";
    case shared::MessageId::kMoveRequest:
      return "MoveRequest";
    case shared::MessageId::kMoveCorrection:
      return "MoveCorrection";
    case shared::MessageId::kPickupRequest:
      return "PickupRequest";
    case shared::MessageId::kPickupResult:
      return "PickupResult";
    case shared::MessageId::kInventoryDelta:
      return "InventoryDelta";
    case shared::MessageId::kCastSkillRequest:
      return "CastSkillRequest";
    case shared::MessageId::kCastSkillResult:
      return "CastSkillResult";
  }

  return "Unknown";
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "usage: proto_dump <message-id>\n";
    return 1;
  }

  try {
    const auto raw_value = static_cast<std::uint16_t>(std::stoul(argv[1]));
    const auto message_id = static_cast<shared::MessageId>(raw_value);
    std::cout << raw_value << ": " << MessageName(message_id) << '\n';
    return 0;
  } catch (const std::exception&) {
    std::cerr << "invalid message id\n";
    return 1;
  }
}
