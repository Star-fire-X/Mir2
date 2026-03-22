#ifndef SHARED_PROTOCOL_AUTH_MESSAGES_H_
#define SHARED_PROTOCOL_AUTH_MESSAGES_H_

#include <string>

#include "shared/protocol/message_ids.h"
#include "shared/types/error_codes.h"
#include "shared/types/player_id.h"

namespace shared {

struct LoginRequest {
  static constexpr MessageId kMessageId = MessageId::kLoginRequest;
  std::string account_name;
  std::string password;
};

struct LoginResponse {
  static constexpr MessageId kMessageId = MessageId::kLoginResponse;
  ErrorCode error_code = ErrorCode::kOk;
  PlayerId player_id;
};

}  // namespace shared

#endif  // SHARED_PROTOCOL_AUTH_MESSAGES_H_
